/**
 * Arduino WiFi Connection Handler
 * Connects to Arduino dial data via Server-Sent Events (SSE)
 * Provides same interface as ArduinoSerial for drop-in replacement
 */

class ArduinoWiFi {
	constructor() {
		this.eventSource = null;
		this.isConnected = false;
		this.dialChangeHandler = null;
		this.clickHandler = null;
		this.lastPosition = 0;
	}
	
	async connect() {
		return new Promise((resolve, reject) => {
			try {
				// Connect to the Server-Sent Events endpoint
				this.eventSource = new EventSource('/api/arduino');
				
				this.eventSource.onopen = () => {
					console.log('Arduino WiFi connection established');
					this.isConnected = true;
					resolve();
				};
				
				this.eventSource.onmessage = (event) => {
					try {
						const data = JSON.parse(event.data);
						this.handleMessage(data);
					} catch (error) {
						console.error('Error parsing Arduino data:', error);
					}
				};
				
				this.eventSource.onerror = (error) => {
					console.error('Arduino WiFi connection error:', error);
					this.isConnected = false;
					if (!this.isConnected) {
						reject(new Error('Failed to connect to Arduino WiFi'));
					}
				};
				
				// Timeout after 10 seconds
				setTimeout(() => {
					if (!this.isConnected) {
						this.eventSource?.close();
						reject(new Error('Arduino WiFi connection timeout'));
					}
				}, 10000);
				
			} catch (error) {
				reject(error);
			}
		});
	}

	async disconnect() {
		if (this.eventSource) {
			this.eventSource.close();
			this.eventSource = null;
		}
		this.isConnected = false;
		console.log('Arduino WiFi disconnected');
	}
	
	setDialChangeHandler(handler) {
		this.dialChangeHandler = handler;
	}
	
	setClickHandler(handler) {
		this.clickHandler = handler;
	}
	
	handleMessage(data) {
		switch (data.type) {
			case 'connected':
				console.log('Arduino WiFi ready');
				break;
				
			case 'dial':
				if (this.dialChangeHandler && data.delta !== undefined) {
					// Create dial data object matching serial format
					const dialData = {
						position: data.position,
						delta: data.delta,
						timestamp: Date.now()
					};
					this.dialChangeHandler(dialData);
				}
				break;
				
			case 'click':
				if (this.clickHandler) {
					this.clickHandler();
				}
				break;
				
			default:
				console.log('Unknown Arduino message type:', data.type);
		}
	}
	
	// Helper method to check connection status
	getConnectionStatus() {
		return {
			connected: this.isConnected,
			type: 'wifi',
			endpoint: '/api/arduino'
		};
	}
}

export default ArduinoWiFi;