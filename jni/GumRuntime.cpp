
#include "GumRuntime.h"
#include "frida-gumjs.h"
#include "agent_blob.h"
#include <android/log.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

#define TAG "AIZ3N_GUM"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static GMainContext *g_context = nullptr;
static GumScriptBackend *g_backend = nullptr;
static GumScript *g_script = nullptr;
static std::thread g_thread;
static std::mutex g_state_mutex;
static std::condition_variable g_state_cv;
static bool g_ready = false;
static bool g_stopping = false;
static bool g_loaded = false;
static bool g_initialized = false;
static void (*g_message_handler)(const char *) = nullptr;

struct Request {
    enum Type {
        LOAD,
        UNLOAD,
        POST
    } type;

    std::string message;
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    bool result = false;
};

static void on_message(
    const gchar *message,
    GBytes *,
    gpointer
) {
    void (*handler)(const char *) = nullptr;

    {
        std::lock_guard<std::mutex> lock(g_state_mutex);
        handler = g_message_handler;
    }

    if (handler) {
        handler(message);
    } else {
        __android_log_print(
            ANDROID_LOG_INFO,
            TAG,
            "%s",
            message
        );
    }
}

static gboolean process_request(gpointer data) {
    Request *request = static_cast<Request *>(data);
    bool result = false;

    if (request->type == Request::LOAD) {
        if (!g_script) {
            GError *error = nullptr;

            std::string source(
                reinterpret_cast<const char *>(agent_js),
                agent_js_len
            );

            g_script = gum_script_backend_create_sync(
                g_backend,
                "AIZ3N-Agent",
                source.c_str(),
                nullptr,
                nullptr,
                &error
            );

            if (g_script) {
                gum_script_set_message_handler(
                    g_script,
                    on_message,
                    nullptr,
                    nullptr
                );

                gum_script_load_sync(g_script, nullptr);
                result = true;
            } else if (error) {
                LOGE(
                    "Script creation failed: %s",
                    error->message
                );

                g_error_free(error);
            }
        } else {
            result = true;
        }

        {
            std::lock_guard<std::mutex> lock(g_state_mutex);
            g_loaded = result;
        }
    } else if (request->type == Request::UNLOAD) {
        if (g_script) {
            gum_script_unload_sync(g_script, nullptr);
            g_object_unref(g_script);
            g_script = nullptr;
        }

        {
            std::lock_guard<std::mutex> lock(g_state_mutex);
            g_loaded = false;
        }

        result = true;
    } else if (request->type == Request::POST) {
        if (g_script) {
            gum_script_post(
                g_script,
                request->message.c_str(),
                nullptr
            );

            result = true;
        }
    }

    {
        std::lock_guard<std::mutex> lock(request->mutex);
        request->result = result;
        request->done = true;
    }

    request->cv.notify_one();

    return G_SOURCE_REMOVE;
}

static bool run_request(Request &request) {
    std::unique_lock<std::mutex> state_lock(g_state_mutex);

    if (!g_initialized || g_stopping || !g_context) {
        return false;
    }

    GSource *source = g_idle_source_new();

    if (!source) {
        return false;
    }

    g_source_set_priority(
        source,
        G_PRIORITY_DEFAULT
    );

    g_source_set_callback(
        source,
        process_request,
        &request,
        nullptr
    );

    guint source_id = g_source_attach(
        source,
        g_context
    );

    g_source_unref(source);

    if (source_id == 0) {
        return false;
    }

    g_main_context_wakeup(g_context);

    state_lock.unlock();

    std::unique_lock<std::mutex> request_lock(request.mutex);

    request.cv.wait(
        request_lock,
        [&request] {
            return request.done;
        }
    );

    return request.result;
}

static void runtime_thread() {
    gum_init_embedded();

    GMainContext *context = g_main_context_new();

    if (!context) {
        gum_deinit_embedded();

        {
            std::lock_guard<std::mutex> lock(g_state_mutex);
            g_ready = true;
            g_initialized = false;
        }

        g_state_cv.notify_all();
        return;
    }

    g_main_context_push_thread_default(context);

    GumScriptBackend *backend =
        gum_script_backend_obtain_qjs();

    {
        std::lock_guard<std::mutex> lock(g_state_mutex);

        g_context = context;
        g_backend = backend;
        g_ready = true;
        g_initialized = backend != nullptr;
    }

    g_state_cv.notify_all();

    if (backend) {
        while (true) {
            while (g_main_context_iteration(context, FALSE)) {
            }

            {
                std::lock_guard<std::mutex> lock(g_state_mutex);

                if (g_stopping) {
                    break;
                }
            }

            g_main_context_iteration(context, TRUE);
        }
    }

    if (g_script) {
        gum_script_unload_sync(g_script, nullptr);
        g_object_unref(g_script);
        g_script = nullptr;
    }

    if (backend) {
        g_object_unref(backend);
    }

    g_main_context_pop_thread_default(context);
    g_main_context_unref(context);

    gum_deinit_embedded();

    {
        std::lock_guard<std::mutex> lock(g_state_mutex);

        g_context = nullptr;
        g_backend = nullptr;
        g_loaded = false;
        g_initialized = false;
    }
}

bool GumRuntime_Init() {
    std::unique_lock<std::mutex> lock(g_state_mutex);

    if (g_initialized) {
        return true;
    }

    if (g_thread.joinable()) {
        return false;
    }

    g_ready = false;
    g_stopping = false;
    g_loaded = false;

    g_thread = std::thread(runtime_thread);

    g_state_cv.wait(
        lock,
        [] {
            return g_ready;
        }
    );

    bool result = g_initialized;

    lock.unlock();

    if (!result && g_thread.joinable()) {
        g_thread.join();
    }

    return result;
}

bool GumRuntime_LoadAgent() {
    Request request;
    request.type = Request::LOAD;

    return run_request(request);
}

void GumRuntime_UnloadAgent() {
    Request request;
    request.type = Request::UNLOAD;

    run_request(request);
}

bool GumRuntime_IsLoaded() {
    std::lock_guard<std::mutex> lock(g_state_mutex);
    return g_loaded;
}

bool GumRuntime_Post(const char *message) {
    if (!message) {
        return false;
    }

    Request request;
    request.type = Request::POST;
    request.message = message;

    return run_request(request);
}

void GumRuntime_SetMessageHandler(
    void (*handler)(const char *message)
) {
    std::lock_guard<std::mutex> lock(g_state_mutex);
    g_message_handler = handler;
}

void GumRuntime_Shutdown() {
    {
        std::lock_guard<std::mutex> lock(g_state_mutex);

        if (!g_thread.joinable()) {
            return;
        }

        g_stopping = true;

        if (g_context) {
            g_main_context_wakeup(g_context);
        }
    }

    g_thread.join();
}
