<script>
	let { title, content, stepNumber, isFirst = false, isLast = false, contentOpacity = 1, headerColor = '#ddd' } = $props();
</script>

<section class="slide">
	{#if title}
		{#if isFirst}
			<h1 style="color: {headerColor}; transition: color 0.3s ease;">{title}</h1>
		{:else}
			<h2 style="color: {headerColor}; transition: color 0.3s ease;">{title}</h2>
		{/if}
	{/if}
	
	{#if typeof content === 'string'}
		<p style="opacity: {contentOpacity}; transition: opacity 0.3s ease;">{content}</p>
	{:else if Array.isArray(content)}
		{#each content as paragraph}
			<p style="opacity: {contentOpacity}; transition: opacity 0.3s ease;">{paragraph}</p>
		{/each}
	{:else}
		<div style="opacity: {contentOpacity}; transition: opacity 0.3s ease;">
			{@render content?.()}
		</div>
	{/if}

</section>

<style>
	.slide {
		text-align: center;
		max-width: 90vw;
		padding: 0.5vh 4vw;
		flex-shrink: 0;
	}

	.slide h1, .slide h2 {
		margin-bottom: 1vh;
		max-width: 90vw;
		white-space: normal;
		overflow: visible;
		text-align: center;
		line-height: 1.05;
		font-family: "National 2 Web", -apple-system, BlinkMacSystemFont, Helvetica, Arial, sans-serif;
	}

	.slide h1 {
		font-size: clamp(2rem, 5vw, 7rem);
		font-weight: 500;
	}

	.slide h2 {
		font-size: clamp(1.6rem, 4.5vw, 6rem);
		font-weight: 500;
	}

	.slide p {
		font-size: clamp(1.25rem, 3vw, 4rem);
		line-height: 1.6;
		margin-bottom: 3vh;
		color: #555;
		max-width: 80vw;
		margin-left: auto;
		margin-right: auto;
		font-family: "Atlas Grotesk", -apple-system, BlinkMacSystemFont, Helvetica, Arial, sans-serif;
	}

	/* Ensure readability on very large screens */
	@media (min-width: 1920px) {
		.slide h1 {
			font-size: clamp(6rem, 8vw, 14rem);
		}

		.slide h2 {
			font-size: clamp(5rem, 7vw, 12rem);
		}

		.slide p {
			font-size: clamp(2rem, 3vw, 5rem);
		}
	}

	/* Ensure readability on very small screens */
	@media (max-width: 480px) {
		.slide h1 {
			font-size: clamp(2rem, 8vw, 3rem);
		}

		.slide h2 {
			font-size: clamp(1.8rem, 7vw, 2.5rem);
		}

		.slide p {
			font-size: clamp(1rem, 3vw, 1.5rem);
		}
	}
</style>