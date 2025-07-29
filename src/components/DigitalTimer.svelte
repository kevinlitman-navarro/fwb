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
		padding: 3rem 4rem;
		border-radius: 1rem;
		border: 4px solid #00ff41;
		box-shadow: 
			0 0 40px rgba(0, 255, 65, 0.4),
			inset 0 0 40px rgba(0, 255, 65, 0.15);
		text-shadow: 0 0 20px rgba(0, 255, 65, 0.9);
		display: flex;
		align-items: center;
		gap: 1rem;
	}

	.time-component {
		display: flex;
		flex-direction: column;
		align-items: center;
		min-width: 120px;
	}

	.time-value {
		font-size: 4rem;
		font-weight: bold;
		line-height: 1;
		letter-spacing: 0.1em;
	}

	.time-label {
		font-size: 1.2rem;
		font-weight: normal;
		opacity: 0.8;
		margin-top: 0.5rem;
		text-transform: lowercase;
	}

	.time-separator {
		font-size: 4rem;
		font-weight: bold;
		opacity: 0.6;
		margin: 0 0.5rem;
		align-self: flex-start;
		margin-top: 0.25rem;
	}

	@media (max-width: 768px) {
		.timer-display {
			padding: 2rem 3rem;
			gap: 0.75rem;
		}
		
		.time-component {
			min-width: 90px;
		}
		
		.time-value {
			font-size: 3rem;
		}
		
		.time-label {
			font-size: 1rem;
		}
		
		.time-separator {
			font-size: 3rem;
		}
	}

	@media (max-width: 480px) {
		.timer-display {
			padding: 1.5rem 2rem;
			gap: 0.5rem;
		}
		
		.time-component {
			min-width: 70px;
		}
		
		.time-value {
			font-size: 2.5rem;
		}
		
		.time-label {
			font-size: 0.9rem;
		}
		
		.time-separator {
			font-size: 2.5rem;
		}
	}
</style>