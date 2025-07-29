<script>
	// shadow, plain, line, bubble
	import wordmark from "$svg/wordmark-shadow.svg";
</script>

<header>
	<div class="wordmark">
		<a href="https://pudding.cool" aria-label="The Pudding" target="_self"
			>{@html wordmark}</a
		>
	</div>
</header>

<style>
	header {
		position: fixed;
		top: 1rem;
		left: 1rem;
		z-index: 100;
		pointer-events: none;
	}

	.wordmark {
		max-width: 8em;
		margin: 0;
		padding: 0;
		transform: rotate(-4deg);
		pointer-events: auto;
	}

	.wordmark a {
		border: none;
		display: block;
		color: var(--color-fg);
	}

	.wordmark a:hover {
		background-color: transparent;
	}
</style>
