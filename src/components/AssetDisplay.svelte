<script>
	let { src, visible = true } = $props();
	
	function getFileType(src) {
		if (!src) return 'image';
		const ext = src.split('.').pop().toLowerCase();
		return ['mp4', 'mov', 'webm'].includes(ext) ? 'video' : 'image';
	}
	
	const fileType = $derived(getFileType(src));
</script>

{#if visible && src}
	<div class="asset-overlay">
		{#if fileType === 'video'}
			<video 
				src={src} 
				autoplay 
				muted 
				loop 
				class="asset-media"
				alt="Historical asset"
			/>
		{:else}
			<img 
				src={src} 
				class="asset-media" 
				alt="Historical asset"
			/>
		{/if}
	</div>
{/if}

<style>
	.asset-overlay {
		position: fixed;
		top: 0;
		left: 0;
		width: 100vw;
		height: 100vh;
		display: flex;
		align-items: center;
		justify-content: center;
		z-index: 999;
		background: rgba(0, 0, 0, 0.1);
		backdrop-filter: blur(2px);
	}
	
	.asset-media {
		max-width: 90vw;
		max-height: 90vh;
		object-fit: contain;
		border-radius: 8px;
		box-shadow: 0 10px 30px rgba(0, 0, 0, 0.3);
	}
</style>