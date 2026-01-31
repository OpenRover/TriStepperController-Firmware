/* ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 * ------------------------------------------------------ */

export interface EventHandlerOptions {
    once?: boolean; // Default false
}

export type UnregisterEventHandler = () => void;

export default function createEvent<Args extends any[] = [], Ret = any>() {
    type Hook = (...args: Args) => Ret;
    const hooks = new Set<Hook>();
    // Register a event handler without replacing existing handlers.
    function register(handler: Hook, options: EventHandlerOptions = {}): UnregisterEventHandler {
        const _handler = handler;
        if (options.once)
            // Make handler self-removing
            handler = ((...args: Args) => {
                hooks.delete(handler);
                return _handler(...args);
            }) as Hook;
        else
            // Make handler unique
            handler = (...args: Args) => _handler(...args);
        hooks.add(handler);
        return () => { hooks.delete(handler) };
    }
    function dispatch(...args: Args): Ret[] {
        return [...hooks].map(hook => hook(...args));
    }
    return Object.assign(register, { dispatch });
}
