<script>
	let { time = $bindable(0) } = $props();
	
	// Format time as HH:MM:SS:MS with individual components
	function formatTime(totalSeconds) {
		const hours = Math.floor(totalSeconds / 3600);
		const minutes = Math.floor((totalSeconds % 3600) / 60);
		const seconds = Math.floor(totalSeconds % 60);
		const milliseconds = Math.floor((totalSeconds % 1) * 1000);
		
		return {
			hours: hours.toString().padStart(2, '0'),
			minutes: minutes.toString().padStart(2, '0'),
			seconds: seconds.toString().padStart(2, '0'),
			milliseconds: milliseconds.toString().padStart(3, '0')
		};
	}
	
	const timeComponents = $derived(formatTime(time));
</script>

<div class="digital-timer">
	<div class="timer-display">
		<div class="time-component">
			<div class="time-value">{timeComponents.hours}</div>
			<div class="time-label">hours</div>
		</div>
		<div class="time-separator">:</div>
		<div class="time-component">
			<div class="time-value">{timeComponents.minutes}</div>
			<div class="time-label">minutes</div>
		</div>
		<div class="time-separator">:</div>
		<div class="time-component">
			<div class="time-value">{timeComponents.seconds}</div>
			<div class="time-label">seconds</div>
		</div>
		<div class="time-separator">:</div>
		<div class="time-component">
			<div class="time-value">{timeComponents.milliseconds}</div>
			<div class="time-label">milliseconds</div>
		</div>
	</div>
</div>

<style>
	.digital-timer {
		pointer-events: none;
		display: flex;
		justify-content: center;
	}

	.timer-display {
		background: rgba(0, 0, 0, 0.9);
		color: #00ff41;
		font-family: 'Courier New', monospace;
		padding: clamp(1.2rem, 3.2vh, 4rem) clamp(1.6rem, 4.8vw, 6.4rem);
		border-radius: clamp(0.4rem, 1.6vw, 1.6rem);
		border: clamp(2px, 0.4vw, 5px) solid #00ff41;
		box-shadow: 
			0 0 clamp(16px, 4vw, 48px) rgba(0, 255, 65, 0.4),
			inset 0 0 clamp(16px, 4vw, 48px) rgba(0, 255, 65, 0.15);
		text-shadow: none;
		display: flex;
		align-items: center;
		gap: clamp(0.4rem, 1.6vw, 1.6rem);
	}

	.time-component {
		display: flex;
		flex-direction: column;
		align-items: center;
		min-width: clamp(56px, 6.4vw, 120px);
	}

	.time-value {
		font-size: clamp(1.6rem, 4.8vw, 6.4rem);
		font-weight: bold;
		line-height: 1;
		letter-spacing: 0.1em;
	}

	.time-label {
		font-size: clamp(0.64rem, 1.6vw, 1.6rem);
		font-weight: normal;
		opacity: 0.8;
		margin-top: clamp(0.2rem, 0.8vw, 0.8rem);
		text-transform: lowercase;
	}

	.time-separator {
		font-size: clamp(1.6rem, 4.8vw, 6.4rem);
		font-weight: bold;
		opacity: 0.6;
		margin: 0 clamp(0.2rem, 0.8vw, 0.8rem);
		align-self: flex-start;
		margin-top: clamp(0.1rem, 0.4vw, 0.4rem);
	}

	/* Responsive design now handled by clamp() functions above */
</style>