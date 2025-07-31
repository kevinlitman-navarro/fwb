/**
 * Arduino Bluetooth BLE Connection Handler
 * Connects to Arduino dial data via Web Bluetooth API
 * Provides same interface as ArduinoSerial and ArduinoWiFi for drop-in replacement
 */

class ArduinoBluetooth {
    constructor() {
        this.device = null;
        this.server = null;
        this.service = null;
        this.positionCharacteristic = null;
        this.clickCharacteristic = null;
        this.isConnected = false;
        this.dialChangeHandler = null;
        this.clickHandler = null;
        this.lastPosition = 0;
        
        // Arduino BLE Service and Characteristic UUIDs (from arduino_triple_dial.ino)
        this.SERVICE_UUID = '12345678-1234-1234-1234-123456789abc';
        this.POSITION_CHAR_UUID = '12345678-1234-1234-1234-123456789abd';
        this.CLICK_CHAR_UUID = '12345678-1234-1234-1234-123456789abe';
    }
    
    async connect() {
        // Check if Web Bluetooth is supported
        if (!navigator.bluetooth) {
            throw new Error('Web Bluetooth API not supported in this browser. Use Chrome, Edge, or Opera.');
        }
        
        try {
            console.log('Requesting Arduino Dial BLE device...');
            
            // Request device with our service UUID
            this.device = await navigator.bluetooth.requestDevice({
                filters: [
                    { name: 'Arduino Dial' },
                    { services: [this.SERVICE_UUID] }
                ],
                optionalServices: [this.SERVICE_UUID]
            });
            
            console.log('Arduino Dial device selected:', this.device.name);
            
            // Listen for disconnect events
            this.device.addEventListener('gattserverdisconnected', this.onDisconnected.bind(this));
            
            // Connect to GATT server
            console.log('Connecting to GATT server...');
            this.server = await this.device.gatt.connect();
            
            // Get the dial service
            console.log('Getting dial service...');
            this.service = await this.server.getPrimaryService(this.SERVICE_UUID);
            
            // Get characteristics
            console.log('Getting characteristics...');
            this.positionCharacteristic = await this.service.getCharacteristic(this.POSITION_CHAR_UUID);
            this.clickCharacteristic = await this.service.getCharacteristic(this.CLICK_CHAR_UUID);
            
            // Start listening for notifications
            await this.startNotifications();
            
            this.isConnected = true;
            console.log('Arduino Bluetooth connection established successfully');
            
            return true;
            
        } catch (error) {
            console.error('Failed to connect to Arduino Bluetooth:', error);
            this.cleanup();
            throw error;
        }
    }
    
    async startNotifications() {
        try {
            // Listen for position changes
            await this.positionCharacteristic.startNotifications();
            this.positionCharacteristic.addEventListener('characteristicvaluechanged', (event) => {
                const position = event.target.value.getInt32(0, true); // little-endian
                this.handlePositionChange(position);
            });
            
            // Listen for click events
            await this.clickCharacteristic.startNotifications();
            this.clickCharacteristic.addEventListener('characteristicvaluechanged', (event) => {
                const clicked = event.target.value.getUint8(0) === 1;
                if (clicked && this.clickHandler) {
                    this.clickHandler();
                }
            });
            
            console.log('BLE notifications started successfully');
            
        } catch (error) {
            console.error('Failed to start BLE notifications:', error);
            throw error;
        }
    }
    
    handlePositionChange(position) {
        if (this.dialChangeHandler) {
            // Calculate delta from last position
            const delta = position - this.lastPosition;
            
            // Only send change if there's actual movement
            if (Math.abs(delta) > 0) {
                console.log('BLE position change:', position, 'delta:', delta);
                
                // Create dial data object matching serial/wifi format
                const dialData = {
                    raw: position,
                    position: position,
                    delta: delta,
                    direction: delta > 0 ? 'clockwise' : 'counterclockwise',
                    timestamp: Date.now()
                };
                
                this.dialChangeHandler(dialData);
                this.lastPosition = position;
            }
        }
    }
    
    onDisconnected() {
        console.log('Arduino Bluetooth device disconnected');
        this.cleanup();
    }
    
    async disconnect() {
        try {
            if (this.positionCharacteristic) {
                await this.positionCharacteristic.stopNotifications();
            }
            if (this.clickCharacteristic) {
                await this.clickCharacteristic.stopNotifications();
            }
            if (this.server && this.server.connected) {
                this.server.disconnect();
            }
        } catch (error) {
            console.warn('Error during Bluetooth disconnect:', error);
        }
        
        this.cleanup();
        console.log('Arduino Bluetooth disconnected');
    }
    
    cleanup() {
        this.device = null;
        this.server = null;
        this.service = null;
        this.positionCharacteristic = null;
        this.clickCharacteristic = null;
        this.isConnected = false;
    }
    
    setDialChangeHandler(handler) {
        this.dialChangeHandler = handler;
    }
    
    setClickHandler(handler) {
        this.clickHandler = handler;
    }
    
    // Helper method to check connection status
    getConnectionStatus() {
        return {
            connected: this.isConnected,
            type: 'bluetooth',
            deviceName: this.device ? this.device.name : null,
            deviceId: this.device ? this.device.id : null
        };
    }
    
    // Check if Web Bluetooth is available
    static isSupported() {
        return 'bluetooth' in navigator;
    }
    
    // Get supported status with user-friendly message
    static getSupportInfo() {
        if (!ArduinoBluetooth.isSupported()) {
            return {
                supported: false,
                message: 'Web Bluetooth not supported. Use Chrome, Edge, or Opera browser.',
                requirements: ['Chrome 56+', 'Edge 79+', 'Opera 43+', 'HTTPS required']
            };
        }
        
        return {
            supported: true,
            message: 'Web Bluetooth supported',
            requirements: ['HTTPS required', 'User gesture required']
        };
    }
}

export default ArduinoBluetooth;