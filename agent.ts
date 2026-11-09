
import "frida-il2cpp-bridge";

let GRAVITY = 0.1;
let enabled = false;
let klass: Il2Cpp.Class;
const hooks: InvocationListener[] = [];

function log(message: string) {
    send({
        type: "log",
        payload: "[AIZ3N] " + message
    });
}

function hookGravity(methodName: string) {
    const method = klass.method(methodName);

    const listener = Interceptor.attach(method.virtualAddress, {
        onEnter(args) {
            (this as InvocationContext & { instance: NativePointer }).instance = args[0];
        },
        onLeave() {
            try {
                const context = this as InvocationContext & { instance: NativePointer };
                const instance = new Il2Cpp.Object(context.instance);
                instance.field<number>("gravityScalef").value = GRAVITY;
            } catch (e) {
                log(String(e));
            }
        }
    });

    hooks.push(listener);
    log("Hooked " + methodName);
}

function enable() {
    if (enabled) return;

    try {
        hookGravity("Awake");
        hookGravity("Start");
        hookGravity("FixedUpdate");
        enabled = true;
        log("Gravity patch enabled: " + GRAVITY);
    } catch (e) {
        disable();
        log("Enable failed: " + String(e));
    }
}

function disable() {
    for (const hook of hooks.splice(0)) {
        hook.detach();
    }

    enabled = false;
    log("Gravity patch disabled");
}

function receiveCommands() {
    recv("command", message => {
        try {
            const command = message.payload;

            switch (command.action) {
                case "enable":
                    enable();
                    break;

                case "disable":
                    disable();
                    break;

                case "set_gravity":
                    if (typeof command.value === "number" &&
                        Number.isFinite(command.value)) {
                        GRAVITY = command.value;
                        log("Gravity value: " + GRAVITY);
                    }
                    break;

                case "status":
                    send({
                        type: "status",
                        payload: {
                            enabled,
                            gravity: GRAVITY
                        }
                    });
                    break;

                default:
                    log("Unknown command");
            }
        } catch (e) {
            log("Command error: " + String(e));
        }

        receiveCommands();
    });
}

Il2Cpp.perform(() => {
    try {
        const image = Il2Cpp.domain.assembly("Assembly-CSharp").image;
        klass = image.class("NinjaMovementScript");

        log("NinjaMovementScript found");
        log("Gravity agent initialized");

        receiveCommands();
    } catch (e) {
        log("Initialization failed: " + String(e));
    }
});
