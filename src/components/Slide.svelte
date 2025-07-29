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
		max-width: 800px;
		padding: 1rem 2rem;
	}

	.slide h1, .slide h2 {
		margin-bottom: 0.5rem;
		max-width: 1200px;
		white-space: nowrap;
		overflow: visible;
	}

	.slide h1 {
		font-size: 4rem;
	}

	.slide h2 {
		font-size: 3.5rem;
	}

	.slide p {
		font-size: 1.5rem;
		line-height: 1.6;
		margin-bottom: 1.5rem;
		color: #555;
		max-width: 800px;
		margin-left: auto;
		margin-right: auto;
	}


	@media (max-width: 768px) {
		.slide h1 {
			font-size: 3rem;
		}

		.slide h2 {
			font-size: 2.5rem;
		}

		.slide p {
			font-size: 1.25rem;
		}

	}

	@media (max-width: 480px) {
		.slide h1 {
			font-size: 2.5rem;
		}

		.slide h2 {
			font-size: 2rem;
		}

		.slide p {
			font-size: 1.125rem;
		}

	}
</style>