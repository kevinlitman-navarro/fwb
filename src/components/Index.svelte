<script>
	import { getContext, onMount, onDestroy } from "svelte";
	import Footer from "$components/Footer.svelte";
	import Slide from "$components/Slide.svelte";
	import BarChart from "$components/BarChart.svelte";
	import DigitalTimer from "$components/DigitalTimer.svelte";
	import RippleEffect from "$components/RippleEffect.svelte";
	import AssetDisplay from "$components/AssetDisplay.svelte";
	import { csv } from "d3-fetch";
	import ArduinoSerial from "$utils/arduinoSerial.js";
	import ArduinoWiFi from "$utils/arduinoWebSocket.js";

	const copy = getContext("copy");
	const data = getContext("data");

	let currentStep = $state(0);
	const totalSteps = 11;
	let chartData = $state([]);
	let timerSeconds = $state(0);
	let rippleTrigger = $state(0);
	let continuousAudio = $state(null);
	
	// Asset cycling state
	let currentAssetIndex = $state(0);
	let currentAssetSrc = $state(null);
	let showAsset = $state(false);
	
	// Page launch timer
	let pageStartTime = $state(null);
	let elapsedTime = $state(0);
	let elapsedTimerInterval = $state(null);
	
	// Map steps to folder names
	const stepToFolder = {
		1: 'intro_1',
		2: '2024', 
		3: '1992',
		4: '1879',
		5: '1843',
		6: '500',
		7: 'babylon'
	};
	
	// Get available assets for current step
	function getAvailableAssets(step) {
		const folderName = stepToFolder[step];
		if (!folderName) return [];
		
		// Based on the folder structure we saw, return expected assets
		const assetMap = {
			'intro_1': ['1.png', '2.png', '3.png', '4.mov', '5.jpg', '6.jpg', '7.png'],
			'2024': ['1.jpeg'],
			'1992': ['1.png'], 
			'1879': ['1.jpg', '2.png', '3.png'],
			'1843': ['1.jpg', '2.jpg'],
			'500': ['1.mp4'],
			'babylon': ['1.jpg']
		};
		
		return assetMap[folderName] || [];
	}
	
	// Get asset source path
	function getAssetSrc(step, assetNumber) {
		const folderName = stepToFolder[step];
		const availableAssets = getAvailableAssets(step);
		const assetIndex = assetNumber - 1;
		
		if (folderName && availableAssets[assetIndex]) {
			return `/assets/images/${folderName}/${availableAssets[assetIndex]}`;
		}
		return null;
	}
	
	// Reset asset state when changing steps
	function resetAssetState() {
		currentAssetIndex = 0;
		currentAssetSrc = null;
		showAsset = false;
	}
	
	// Format elapsed time for display
	function formatElapsedTime(seconds) {
		const minutes = Math.floor(seconds / 60);
		const remainingSeconds = Math.floor(seconds % 60);
		
		if (minutes === 0) {
			return `${remainingSeconds} second${remainingSeconds !== 1 ? 's' : ''}`;
		} else if (minutes === 1) {
			return `1 minute and ${remainingSeconds} second${remainingSeconds !== 1 ? 's' : ''}`;
		} else {
			return `${minutes} minutes and ${remainingSeconds} second${remainingSeconds !== 1 ? 's' : ''}`;
		}
	}
	
	// Start elapsed time tracking
	function startElapsedTimer() {
		pageStartTime = Date.now();
		elapsedTimerInterval = setInterval(() => {
			elapsedTime = (Date.now() - pageStartTime) / 1000; // Convert to seconds
		}, 100); // Update every 100ms for smooth updates
	}
	
	// Calculate light earned in each era based on elapsed time
	function calculateLightEarned(elapsedSeconds) {
		if (!chartData || chartData.length === 0 || elapsedSeconds <= 0) return [];
		
		return chartData.map(era => {
			const laborPriceSeconds = era.labor_price_seconds;
			const hoursOfLight = elapsedSeconds / laborPriceSeconds;
			
			let formattedAmount;
			if (hoursOfLight < 0.01) {
				// Very small amounts - show in scientific notation or as fraction
				formattedAmount = hoursOfLight.toExponential(2);
			} else if (hoursOfLight < 1) {
				// Less than 1 hour - show minutes
				const minutes = hoursOfLight * 60;
				formattedAmount = `${minutes.toFixed(1)} minutes`;
			} else if (hoursOfLight < 24) {
				// Less than 24 hours - show hours
				formattedAmount = `${hoursOfLight.toFixed(1)} hours`;
			} else if (hoursOfLight < 8760) {
				// Less than 1 year - show days
				const days = hoursOfLight / 24;
				formattedAmount = `${days.toFixed(1)} days`;
			} else {
				// More than 1 year - show years
				const years = hoursOfLight / 8760;
				formattedAmount = `${years.toFixed(1)} years`;
			}
			
			return {
				year: era.year,
				light: era.light,
				hoursOfLight,
				formattedAmount
			};
		}).sort((a, b) => +b.year - +a.year); // Sort by year descending (newest first)
	}
	
	// Arduino dial connection
	let arduino = $state(null);
	let isArduinoConnected = $state(false);
	let arduinoError = $state(null);
	let connectionType = $state('serial'); // 'serial' or 'wifi'

	// Calculate timer limit based on labor price for current era
	function getTimerLimit(step) {
		if (!chartData || chartData.length === 0) return 60; // fallback
		
		const dataIndex = step - 2; // step 2 = index 0
		if (dataIndex < 0 || dataIndex >= chartData.length) return 60;
		
		const era = chartData[dataIndex];
		if (!era) return 60;
		
		const stepPosition = step - 2; // 0-5 for the 6 eras
		let limit;
		
		if (stepPosition <= 1) {
			// First two eras (2024, 1992): use seconds as-is
			limit = era.labor_price_seconds;
		} else if (stepPosition === 2) {
			// Third era (1879): convert minutes to seconds (45 minutes = 2700 seconds)
			limit = era.labor_price_minutes * 60;
		} else {
			// Last three eras (1843, 500 CE, 1750 BCE): convert hours to seconds
			limit = era.labor_price_hours * 3600;
		}
		
		// Ensure we have a valid number
		if (isNaN(limit) || limit <= 0) {
			console.warn(`Invalid timer limit for step ${step}:`, limit, era);
			return 60;
		}
		
		return Math.max(limit, 0.01); // minimum 0.01 to avoid zero
	}

	const currentTimerLimit = $derived(getTimerLimit(currentStep));

	// Optimize derived calculations with memoization
	const backgroundBrightness = $derived(Math.min(timerSeconds / currentTimerLimit, 1));
	
	// Force light styling after Babylon section (step 7)
	const isLightMode = $derived(currentStep > 7);
	const effectiveBrightness = $derived(isLightMode ? 1 : backgroundBrightness);
	
	// Direct derived calculations for responsive Arduino control
	const backgroundColor = $derived(`hsl(0, 0%, ${effectiveBrightness * 100}%)`);
	const textColor = $derived(effectiveBrightness > 0.5 ? '#000000' : '#ffffff');
	const headerColor = $derived(effectiveBrightness > 0.7 ? '#333333' : '#dddddd');
	const textShadow = $derived(effectiveBrightness < 0.5 
		? '0 0 10px rgba(255, 255, 255, 0.8), 0 0 20px rgba(255, 255, 255, 0.6), 0 0 30px rgba(255, 255, 255, 0.4)'
		: '0 2px 4px rgba(0, 0, 0, 0.3)');
	const chartOpacity = $derived(isLightMode ? 1 : backgroundBrightness);

	// Use effect to reset timer when entering year steps (to avoid race conditions)
	$effect(() => {
		if (currentStep >= 2 && currentStep <= 7) {
			console.log(`Effect: Resetting timer for step ${currentStep}, limit: ${currentTimerLimit}`);
			console.log(`Chart data available:`, chartData?.length > 0, chartData?.[currentStep - 2]);
			
			// Cancel any ongoing animations when switching steps
			if (currentAnimationId) {
				cancelAnimationFrame(currentAnimationId);
				currentAnimationId = null;
			}
			if (wheelAnimationId) {
				cancelAnimationFrame(wheelAnimationId);
				wheelAnimationId = null;
			}
			
			timerSeconds = 0;
		}
	});

	// Load the CSV data
	async function loadData() {
		try {
			const rawData = await csv("/assets/data/data.csv");
			chartData = rawData.map(d => ({
				year: d.year,
				labor_price_seconds: +d.labor_price_seconds,
				labor_price_minutes: +d.labor_price_minutes,
				labor_price_hours: +d.labor_price_hours,
				light: d.light
			})).sort((a, b) => +b.year - +a.year); // Sort by year descending (newest first)
			console.log("Loaded chart data:", chartData);
		} catch (error) {
			console.error("Error loading data:", error);
		}
	}

	// Arduino connection functions
	async function connectArduino(type = connectionType) {
		try {
			// Create appropriate Arduino connection based on type
			if (type === 'wifi') {
				arduino = new ArduinoWiFi();
			} else {
				arduino = new ArduinoSerial();
			}
			
			await arduino.connect();
			
			// Set up dial change handler
			arduino.setDialChangeHandler((dialData) => {
				handleArduinoDial(dialData);
			});
			
			isArduinoConnected = true;
			arduinoError = null;
			connectionType = type;
			console.log(`Arduino connected via ${type}`);
		} catch (error) {
			arduinoError = error.message;
			console.error('Arduino connection failed:', error);
		}
	}

	async function disconnectArduino() {
		if (arduino) {
			await arduino.disconnect();
			arduino = null;
		}
		isArduinoConnected = false;
		arduinoError = null;
	}

	function handleArduinoDial(dialData) {
		// Only respond to dial input if we're on timer steps (2-7)
		if (currentStep < 2 || currentStep > 7) return;
		
		// Convert dial delta to proportional scroll amount
		const sensitivity = 3; // Increased sensitivity for WiFi connection
		const scrollAmount = dialData.delta * sensitivity;
		
		// Create synthetic wheel event with proportional deltaY
		const syntheticEvent = {
			deltaY: scrollAmount,
			preventDefault: () => {}
		};
		
		// Use existing wheel handler logic with reduced debouncing for WiFi
		handleWheelImmediate(syntheticEvent);
	}

	// Immediate wheel handler for Arduino (bypasses debouncing)
	function handleWheelImmediate(event) {
		// Cancel any existing animation
		if (wheelAnimationId) {
			cancelAnimationFrame(wheelAnimationId);
			wheelAnimationId = null;
		}
		
		// For step 2 (2024) with very small timer limits, use direct updates
		if (currentStep === 2 && currentTimerLimit < 1) {
			// Since 2024 step has 0.01s limit and 10ms minimum movement,
			// any forward movement should complete the timer
			if (event.deltaY > 0) {
				timerSeconds = currentTimerLimit; // Jump to end
			} else if (event.deltaY < 0) {
				timerSeconds = 0; // Jump to start
			}
			return;
		}
		
		// Calculate appropriate increment
		let increment;
		if (currentTimerLimit < 1) {
			increment = currentTimerLimit / 50;
		} else if (currentTimerLimit < 60) {
			increment = currentTimerLimit / 15;
		} else {
			increment = currentTimerLimit / 30;
		}
		
		// Adjust increments for each step
		if (currentStep === 2) {
			increment = Math.max(increment, 0.01); // 10ms minimum (instant completion)
		} else if (currentStep === 3) {
			increment = Math.max(increment, 0.21); // 210ms minimum (completes in 2 movements)
		} else if (currentStep === 4) {
			increment = increment / 2; // 2x longer (1879: Edison's bulbs)
		} else if (currentStep === 5) {
			increment = increment / 3; // 3x longer (1843: Whale oil)
		} else if (currentStep === 6) {
			increment = increment / 10; // 10x longer (500 CE: Olive oil)
		} else if (currentStep === 7) {
			increment = increment / 20; // 20x longer (1750 BCE: Sesame oil)
		}
		
		if (event.deltaY > 0) {
			animateTimer(timerSeconds, Math.min(timerSeconds + increment, currentTimerLimit));
		} else if (event.deltaY < 0) {
			animateTimer(timerSeconds, Math.max(timerSeconds - increment, 0));
		}
	}

	// Load data when component mounts
	onMount(() => {
		loadData();
		startElapsedTimer();
	});

	// Cleanup on component destroy
	onDestroy(() => {
		if (arduino) {
			disconnectArduino();
		}
		if (elapsedTimerInterval) {
			clearInterval(elapsedTimerInterval);
		}
	});

	function nextStep() {
		// Check if current step has assets to cycle through
		const availableAssets = getAvailableAssets(currentStep);
		
		if (availableAssets.length > 0) {
			// If we haven't started showing assets yet, show the first one
			if (currentAssetIndex === 0 && !showAsset) {
				currentAssetIndex = 1;
				currentAssetSrc = getAssetSrc(currentStep, 1);
				showAsset = true;
				return;
			}
			
			// If we're showing an asset, try to show the next one
			if (showAsset && currentAssetIndex < availableAssets.length) {
				currentAssetIndex++;
				const nextAssetSrc = getAssetSrc(currentStep, currentAssetIndex);
				
				if (nextAssetSrc) {
					// Show next asset (replaces current)
					currentAssetSrc = nextAssetSrc;
					return;
				}
			}
			
			// If we're at the last asset or beyond, remove the current asset
			if (showAsset) {
				showAsset = false;
				currentAssetSrc = null;
				return;
			}
		}
		
		// No assets or all assets have been cycled through, advance to next step
		if (currentStep < totalSteps - 1) {
			rippleTrigger++;
			playStepSound(currentStep, currentStep + 1);
			currentStep++;
			resetAssetState();
		}
	}

	function playStepSound(fromStep, toStep) {
		// Stop any continuous audio from previous step
		if (continuousAudio) {
			continuousAudio.pause();
			continuousAudio = null;
		}
		
		// Only play audio starting from step 1 ("A Journey Back in Time") to step 2 (2024) onwards
		if (fromStep < 1) {
			return; // No sound for transitions before step 1
		}
		
		// Special cases: continuous audio for historical eras
		if (toStep === 5) {
			// Victorian England (1843: The Age of Whale Oil)
			continuousAudio = new Audio('/assets/sounds/victorian.mp3');
			continuousAudio.loop = true;
			continuousAudio.play().catch(e => console.log('Victorian audio play failed:', e));
			return;
		} else if (toStep === 6) {
			// Rome (500 CE: Classical Olive Oil)
			continuousAudio = new Audio('/assets/sounds/rome.mp3');
			continuousAudio.loop = true;
			continuousAudio.play().catch(e => console.log('Rome audio play failed:', e));
			return;
		} else if (toStep === 7) {
			// Babylon (1750 BCE: Ancient Sesame Oil)
			continuousAudio = new Audio('/assets/sounds/gish.mp3');
			continuousAudio.loop = true;
			continuousAudio.play().catch(e => console.log('Gish audio play failed:', e));
			return;
		}
		
		// Play audio for forward transitions
		let audioFile;
		
		// Special case: step 2 (2024) to step 3 (1992) - use seinfeld sound
		if (fromStep === 2 && toStep === 3) {
			audioFile = '/assets/sounds/seinfeld.mp3';
		} else {
			// All other forward transitions use flashback sound
			audioFile = '/assets/sounds/flashback.mp3';
		}
		
		const audio = new Audio(audioFile);
		audio.currentTime = 0; // Start from beginning
		
		// Play for 10 seconds then stop (7 seconds for seinfeld, 10 for flashback)
		const duration = audioFile.includes('seinfeld') ? 7000 : 10000;
		audio.play().catch(e => console.log('Audio play failed:', e));
		
		setTimeout(() => {
			audio.pause();
			audio.currentTime = 0;
		}, duration);
	}

	function prevStep() {
		if (currentStep > 0) {
			rippleTrigger++;
			// Stop continuous audio when leaving any step
			if (continuousAudio) {
				continuousAudio.pause();
				continuousAudio = null;
			}
			currentStep--;
			resetAssetState();
		}
	}


	function handleClick(event) {
		// Only advance on clicks to the main content area, not UI elements
		if (event.target.closest('.stepper-ui')) return;
		nextStep();
	}

	function handleKeydown(event) {
		if (event.key === 'ArrowRight') {
			nextStep();
		} else if (event.key === 'ArrowLeft') {
			prevStep();
		}
	}

	let wheelAnimationId = null;
	
	function handleWheel(event) {
		event.preventDefault();
		
		// Cancel any existing animation
		if (wheelAnimationId) {
			cancelAnimationFrame(wheelAnimationId);
			wheelAnimationId = null;
		}
		
		// For step 2 (2024) with very small timer limits, use direct updates without animation
		if (currentStep === 2 && currentTimerLimit < 1) {
			// Since 2024 step has 0.01s limit and 10ms minimum movement,
			// any forward movement should complete the timer
			if (event.deltaY > 0) {
				timerSeconds = currentTimerLimit; // Jump to end
			} else if (event.deltaY < 0) {
				timerSeconds = 0; // Jump to start
			}
			return;
		}
		
		// Debounce wheel events for other steps
		if (handleWheel.timeout) {
			clearTimeout(handleWheel.timeout);
		}
		
		handleWheel.timeout = setTimeout(() => {
			// Calculate appropriate increment based on timer limit
			let increment;
			if (currentTimerLimit < 1) {
				increment = currentTimerLimit / 50; // More precise for sub-second
			} else if (currentTimerLimit < 60) {
				increment = currentTimerLimit / 15; // Smoother for small eras
			} else {
				increment = currentTimerLimit / 30; // Standard for large eras
			}
			
			// Adjust increments for each step
			if (currentStep === 2) {
				increment = Math.max(increment, 0.01); // 10ms minimum (instant completion)
			} else if (currentStep === 3) {
				increment = Math.max(increment, 0.21); // 210ms minimum (completes in 2 movements)
			} else if (currentStep === 4) {
				increment = increment / 2; // 2x longer (1879: Edison's bulbs)
			} else if (currentStep === 5) {
				increment = increment / 3; // 3x longer (1843: Whale oil)
			} else if (currentStep === 6) {
				increment = increment / 10; // 10x longer (500 CE: Olive oil)
			} else if (currentStep === 7) {
				increment = increment / 20; // 20x longer (1750 BCE: Sesame oil)
			}
			
			if (event.deltaY > 0) {
				// Scrolling down - increment timer with animation
				animateTimer(timerSeconds, Math.min(timerSeconds + increment, currentTimerLimit));
			} else if (event.deltaY < 0) {
				// Scrolling up - decrement timer with animation
				animateTimer(timerSeconds, Math.max(timerSeconds - increment, 0));
			}
		}, 16); // Optimize to ~60fps
	}

	let currentAnimationId = null;
	
	function animateTimer(start, end) {
		// Cancel any existing animation
		if (currentAnimationId) {
			cancelAnimationFrame(currentAnimationId);
		}
		
		// Skip animation for very small differences (< 0.001)
		if (Math.abs(end - start) < 0.001) {
			timerSeconds = parseFloat(end.toFixed(3));
			checkTimerComplete(timerSeconds);
			return;
		}
		
		// Shorter duration for step 2 (2024) for better responsiveness
		const duration = currentStep === 2 ? 150 : 300;
		const startTime = performance.now();
		
		function animate(currentTime) {
			const elapsed = currentTime - startTime;
			const progress = Math.min(elapsed / duration, 1);
			
			if (progress < 1) {
				// Use simpler easing for better performance
				const eased = progress * progress * (3 - 2 * progress); // smoothstep
				timerSeconds = parseFloat((start + (end - start) * eased).toFixed(3));
				currentAnimationId = requestAnimationFrame(animate);
			} else {
				timerSeconds = parseFloat(end.toFixed(3));
				checkTimerComplete(timerSeconds);
				currentAnimationId = null;
			}
		}
		
		currentAnimationId = requestAnimationFrame(animate);
	}

	function checkTimerComplete(currentTime) {
		// Check if timer has reached the limit (within 1% tolerance)
		if (currentStep >= 2 && currentStep <= 7) {
			const completion = currentTime / currentTimerLimit;
			if (completion >= 0.99) {
				playDingSound();
			}
		}
	}

	function playDingSound() {
		// Try to play ding sound file, fallback to programmatic beep
		const ding = new Audio('/assets/sounds/ding.mp3');
		ding.volume = 0.3; // Gentle volume
		ding.play().catch(e => {
			console.log('Ding sound file not found, using fallback beep');
			// Fallback: create a simple beep using Web Audio API
			playBeep();
		});
	}

	function playBeep() {
		try {
			const audioContext = new (window.AudioContext || window.webkitAudioContext)();
			const oscillator = audioContext.createOscillator();
			const gainNode = audioContext.createGain();
			
			oscillator.connect(gainNode);
			gainNode.connect(audioContext.destination);
			
			oscillator.frequency.setValueAtTime(800, audioContext.currentTime); // 800Hz tone
			gainNode.gain.setValueAtTime(0.1, audioContext.currentTime); // Low volume
			gainNode.gain.exponentialRampToValueAtTime(0.001, audioContext.currentTime + 0.3);
			
			oscillator.start(audioContext.currentTime);
			oscillator.stop(audioContext.currentTime + 0.3);
		} catch (error) {
			console.log('Audio not available:', error);
		}
	}

	// Slides - chart appears on steps 2-7, intro and conclusion on other steps
	const slides = [
		{
			title: "Earning The Midnight Oil",
			content: "Light after dark - one of humanity's most basic needs. But how much did it really cost throughout history?"
		},
		{
			title: "A Journey Back in Time",
			content: "We're about to travel backwards through time, starting with today's incredibly cheap light and moving back to ancient civilizations. Prepare to see some dramatic differences."
		},
		{
			title: "Today (2024): LED Technology",
			content: "Modern LED lights cost almost nothing in terms of labor - just 0.01 seconds of work for equivalent light."
		},
		{
			title: "1992: Fluorescent Era", 
			content: "Fluorescent bulbs were a major improvement, but still cost 42 times more labor than today's LEDs."
		},
		{
			title: "1879: Dawn of Electric Light",
			content: "Edison's carbon filament bulbs required 2,700 seconds of labor - that's 45 minutes of work for the same light!"
		},
		{
			title: "1843: The Age of Whale Oil",
			content: "Whale oil lamps needed 5 hours of labor (18,000 seconds) for equivalent illumination."
		},
		{
			title: "500 CE: Classical Olive Oil",
			content: "In the classical period, olive oil lamps required 22 hours of labor - nearly a full day's work!"
		},
		{
			title: "1750 BCE: Ancient Sesame Oil",
			content: "In ancient times, sesame oil lamps needed 41 hours of labor. Light was incredibly precious."
		},
		{
			title: "The Transformation",
			content: "We've just witnessed a 14.7 million-fold improvement in the labor cost of light over human history."
		},
		{
			title: "What This Means", 
			content: "Technology has made one of humanity's most basic needs - light after dark - essentially free."
		},
		{
			title: "The End",
			content: "Thank you for this journey through time. The next time you flip a light switch, remember how remarkable that simple act truly is."
		}
	];
</script>

<svelte:window onkeydown={handleKeydown} />

<svelte:boundary onerror={(e) => console.error(e)}>
	<div 
		class="slideshow" 
		onclick={handleClick}
		onwheel={handleWheel}
		style="background-color: {backgroundColor}; color: {textColor}; text-shadow: {textShadow}; transition: background-color 0.3s ease, color 0.3s ease, text-shadow 0.3s ease;"
	>
		<!-- Arduino Connection UI -->
		<div class="arduino-controls">
			{#if !isArduinoConnected}
				<div class="connection-type-selector">
					<label style="color: {textColor}; font-size: 12px;">
						<input 
							type="radio" 
							bind:group={connectionType} 
							value="serial"
						/> USB Serial
					</label>
					<label style="color: {textColor}; font-size: 12px;">
						<input 
							type="radio" 
							bind:group={connectionType} 
							value="wifi"
						/> WiFi
					</label>
				</div>
				<button 
					class="arduino-button connect" 
					onclick={(e) => { e.stopPropagation(); connectArduino(connectionType); }}
					style="color: {textColor}; border-color: {textColor};"
				>
					Connect Arduino ({connectionType.toUpperCase()})
				</button>
			{:else}
				<button 
					class="arduino-button disconnect" 
					onclick={(e) => { e.stopPropagation(); disconnectArduino(); }}
					style="color: {textColor}; border-color: {textColor};"
				>
					Disconnect Arduino
				</button>
				<span class="arduino-status">🔗 Arduino Connected ({connectionType.toUpperCase()})</span>
			{/if}
			
			{#if arduinoError}
				<div class="arduino-error" style="color: #ff6b6b;">
					Error: {arduinoError}
				</div>
			{/if}
		</div>
		
		<RippleEffect bind:trigger={rippleTrigger} />
		<AssetDisplay src={currentAssetSrc} visible={showAsset} />

		<div class="slide-content">
			{#if currentStep >= 2 && currentStep <= 7}
				<!-- Show chart for steps 2-7 (6 chart steps total) -->
				<Slide 
					title={slides[currentStep].title}
					content=""
					stepNumber={currentStep + 1}
					isFirst={false}
					isLast={false}
					contentOpacity={chartOpacity}
					headerColor={headerColor}
				/>
				{#if currentStep >= 2}
					<div class="timer-container">
						<DigitalTimer bind:time={timerSeconds} />
					</div>
				{/if}
				{#if chartData.length > 0}
					<div class="chart-container">
						<BarChart 
							data={chartData} 
							step={currentStep - 2} 
							progress={chartOpacity}
							backgroundColor={backgroundColor}
							textColor={textColor}
							headerColor={headerColor}
						/>
					</div>
				{/if}
			{:else}
				<!-- Regular slides for intro and conclusion steps -->
				<Slide 
					title={slides[currentStep].title}
					content={slides[currentStep].content}
					stepNumber={currentStep + 1}
					isFirst={currentStep === 0}
					isLast={currentStep === totalSteps - 1}
					contentOpacity={chartOpacity}
					headerColor={headerColor}
				/>
				{#if currentStep === totalSteps - 1}
					<div class="elapsed-time" style="color: {textColor}; transition: color 0.3s ease;">
						You experienced this journey through time in {formatElapsedTime(elapsedTime)}.
					</div>
					<div class="light-calculations" style="color: {textColor}; transition: color 0.3s ease;">
						<p style="margin-bottom: 1rem; font-weight: 600;">In that time, you would have earned:</p>
						{#each calculateLightEarned(elapsedTime) as era}
							<p class="light-era">
								<strong>{era.year} ({era.light}):</strong> {era.formattedAmount} of light
							</p>
						{/each}
					</div>
				{/if}
			{/if}
			
		</div>
	</div>
</svelte:boundary>

<style>
	.slideshow {
		min-height: 100vh;
		cursor: pointer;
		position: relative;
	}


	.slide-content {
		min-height: 100vh;
		display: flex;
		flex-direction: column;
		align-items: center;
		justify-content: center;
	}

	.chart-container {
		width: 100%;
		display: flex;
		justify-content: center;
	}

	.timer-container {
		width: 100%;
		display: flex;
		justify-content: center;
		margin: 0.5rem 0;
	}

	.arduino-controls {
		position: fixed;
		top: 20px;
		right: 20px;
		z-index: 1000;
		display: flex;
		flex-direction: column;
		gap: 10px;
		align-items: flex-end;
	}

	.connection-type-selector {
		display: flex;
		gap: 15px;
		margin-bottom: 5px;
	}

	.connection-type-selector label {
		display: flex;
		align-items: center;
		gap: 5px;
		cursor: pointer;
	}

	.connection-type-selector input[type="radio"] {
		margin: 0;
	}

	.arduino-button {
		background: transparent;
		border: 2px solid;
		padding: 8px 16px;
		border-radius: 4px;
		cursor: pointer;
		font-size: 14px;
		transition: all 0.3s ease;
		font-family: inherit;
	}

	.arduino-button:hover {
		background: rgba(255, 255, 255, 0.1);
	}

	.arduino-button.connect {
		border-color: #4CAF50;
	}

	.arduino-button.disconnect {
		border-color: #f44336;
	}

	.arduino-status {
		font-size: 12px;
		opacity: 0.8;
	}

	.arduino-error {
		font-size: 12px;
		background: rgba(255, 107, 107, 0.1);
		padding: 4px 8px;
		border-radius: 4px;
		border: 1px solid #ff6b6b;
		max-width: 200px;
		text-align: right;
	}

	.elapsed-time {
		font-size: 1.2rem;
		font-style: italic;
		text-align: center;
		margin-top: 2rem;
		opacity: 0.8;
	}

	.light-calculations {
		max-width: 600px;
		margin: 2rem auto;
		text-align: center;
		font-size: 1rem;
		line-height: 1.6;
	}

	.light-era {
		margin: 0.5rem 0;
		opacity: 0.9;
	}
</style>
