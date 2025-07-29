/**
 * Arduino Modulino Dial Serial Connection Utility
 * Connects to Arduino via Web Serial API and reads dial input
 */

class ArduinoSerial {
    constructor() {
        this.port = null;
        this.reader = null;
        this.isConnected = false;
        this.onDialChange = null;
        this.lastValue = 0;
    }

    async connect() {
        if (!('serial' in navigator)) {
            throw new Error('Web Serial API not supported in this browser');
        }

        try {
            // Request port access
            this.port = await navigator.serial.requestPort();
            
            // Open connection with standard Arduino settings
            await this.port.open({ 
                baudRate: 9600,
                dataBits: 8,
                stopBits: 1,
                parity: 'none'
            });

            this.isConnected = true;
            this.startReading();
            
            return true;
        } catch (error) {
            console.error('Failed to connect to Arduino:', error);
            throw error;
        }
    }

    async disconnect() {
        if (this.reader) {
            await this.reader.cancel();
            this.reader = null;
        }
        
        if (this.port) {
            await this.port.close();
            this.port = null;
        }
        
        this.isConnected = false;
    }

    async startReading() {
        if (!this.port) return;

        const textDecoder = new TextDecoderStream();
        const readableStreamClosed = this.port.readable.pipeTo(textDecoder.writable);
        this.reader = textDecoder.readable.getReader();

        let buffer = '';

        try {
            while (true) {
                const { value, done } = await this.reader.read();
                if (done) break;

                buffer += value;
                
                // Process complete lines
                const lines = buffer.split('\n');
                buffer = lines.pop(); // Keep incomplete line in buffer

                for (const line of lines) {
                    this.processLine(line.trim());
                }
            }
        } catch (error) {
            console.error('Error reading from Arduino:', error);
        } finally {
            this.reader.releaseLock();
        }
    }

    processLine(line) {
        if (!line) return;

        console.log('Arduino data received:', line); // DEBUG

        try {
            // Expect format: "DIAL:value" where value is from knob position
            if (line.startsWith('DIAL:')) {
                const rawValue = parseInt(line.substring(5));
                console.log('Parsed dial value:', rawValue); // DEBUG
                
                if (!isNaN(rawValue)) {
                    // Calculate delta from last raw value for scroll events
                    const delta = rawValue - this.lastValue;
                    console.log('Delta:', delta, 'Last:', this.lastValue); // DEBUG
                    
                    if (Math.abs(delta) > 0 && this.onDialChange) { // Send every change for responsive control
                        console.log('Sending dial change event'); // DEBUG
                        this.onDialChange({
                            raw: rawValue,
                            delta: delta,
                            direction: delta > 0 ? 'clockwise' : 'counterclockwise'
                        });
                    }
                    
                    this.lastValue = rawValue;
                }
            }
        } catch (error) {
            console.error('Error processing line:', line, error);
        }
    }

    setDialChangeHandler(callback) {
        this.onDialChange = callback;
    }
}

export default ArduinoSerial;