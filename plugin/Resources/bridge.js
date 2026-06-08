// img2sample bridge — thin wrapper over JUCE WebBrowserComponent native functions
// This detects whether we're running inside the JUCE plugin or in a regular browser.

const isPlugin = typeof window.__JUCE__ !== 'undefined';

const bridge = {
    // Call a C++ native function by name. Returns a Promise.
    call(name, ...args) {
        if (!isPlugin) {
            console.warn(`bridge.call('${name}') — not running in plugin context`);
            return Promise.resolve(null);
        }
        return window.__JUCE__.getNativeFunction(name)(...args);
    },

    // Listen for events emitted from C++ via emitEventIfBrowserIsVisible
    on(event, callback) {
        if (!isPlugin) return;
        window.__JUCE__.backend.addEventListener(event, callback);
    },

    // Check if we're running inside the plugin
    get isPlugin() {
        return isPlugin;
    }
};

window.bridge = bridge;
