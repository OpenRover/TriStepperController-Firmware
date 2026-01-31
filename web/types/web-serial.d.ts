// Type definitions for Web Serial API
// https://wicg.github.io/serial/

interface SerialPort extends EventTarget {
    readonly readable: ReadableStream<Uint8Array> | null
    readonly writable: WritableStream<Uint8Array> | null

    open(options: SerialOptions): Promise<void>
    close(): Promise<void>

    forget(): Promise<void>
    getInfo(): SerialPortInfo

    setSignals(signals: SerialOutputSignals): Promise<void>
    getSignals(): Promise<SerialInputSignals>

    addEventListener(
        type: 'connect' | 'disconnect',
        listener: (this: SerialPort, ev: Event) => any,
        options?: boolean | AddEventListenerOptions
    ): void
    removeEventListener(
        type: 'connect' | 'disconnect',
        listener: (this: SerialPort, ev: Event) => any,
        options?: boolean | EventListenerOptions
    ): void
}

interface SerialPortInfo {
    usbVendorId?: number
    usbProductId?: number
}

interface SerialOptions {
    baudRate: number
    dataBits?: 7 | 8
    stopBits?: 1 | 2
    parity?: 'none' | 'even' | 'odd'
    bufferSize?: number
    flowControl?: 'none' | 'hardware'
}

interface SerialOutputSignals {
    dataTerminalReady?: boolean
    requestToSend?: boolean
    break?: boolean
}

interface SerialInputSignals {
    dataCarrierDetect: boolean
    clearToSend: boolean
    ringIndicator: boolean
    dataSetReady: boolean
}

interface SerialPortRequestOptions {
    filters?: SerialPortFilter[]
}

interface SerialPortFilter {
    usbVendorId?: number
    usbProductId?: number
}

interface Serial extends EventTarget {
    getPorts(): Promise<SerialPort[]>
    requestPort(options?: SerialPortRequestOptions): Promise<SerialPort>

    addEventListener(
        type: 'connect' | 'disconnect',
        listener: (this: Serial, ev: Event) => any,
        options?: boolean | AddEventListenerOptions
    ): void
    removeEventListener(
        type: 'connect' | 'disconnect',
        listener: (this: Serial, ev: Event) => any,
        options?: boolean | EventListenerOptions
    ): void
}

interface Navigator {
    readonly serial: Serial
}
