<script>
	let { trigger = $bindable(0) } = $props();
	
	let isAnimating = $state(false);
	let lastTrigger = $state(0);
	
	// Start animation when trigger value changes
	$effect(() => {
		if (trigger > lastTrigger) {
			lastTrigger = trigger;
			isAnimating = true;
		}
	});
</script>

{#if isAnimating}
	<div class="ripple-container">
		{#key trigger}
			<div 
				class="ripple"
				onanimationend={() => isAnimating = false}
			></div>
		{/key}
	</div>
{/if}

<style>
	.ripple-container {
		position: fixed;
		top: 0;
		left: 0;
		width: 100vw;
		height: 100vh;
		pointer-events: none;
		z-index: 1000;
		display: flex;
		align-items: center;
		justify-content: center;
	}

	.ripple {
		width: 20px;
		height: 20px;
		border: 2px solid rgba(255, 255, 255, 0.8);
		border-radius: 50%;
		animation: ripple 3s ease-out forwards;
	}

	@keyframes ripple {
		0% {
			width: 20px;
			height: 20px;
			opacity: 1;
			border-width: 2px;
		}
		50% {
			opacity: 0.6;
			border-width: 1px;
		}
		100% {
			width: 200vmax;
			height: 200vmax;
			opacity: 0;
			border-width: 0px;
		}
	}
</style>