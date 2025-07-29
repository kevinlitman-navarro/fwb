<script>
	import { scaleLinear, scaleBand } from "d3-scale";
	import { max } from "d3-array";
	
	let { data = [], step = 0, progress = 1, backgroundColor = '#ffffff', textColor = '#000000', headerColor = '#333333' } = $props();
	
	// Only show data up to the current step
	const visibleData = $derived(data.slice(0, step + 1));
	
	// Dynamic max based on currently visible data for the shrinking effect
	const targetMax = $derived(max(visibleData, d => d.labor_price_seconds) || 0);
	
	// Interpolate the max value based on progress for smooth axis transitions
	// Start from 10% of target and grow to full target
	const currentMax = $derived(targetMax > 0 ? (targetMax * 0.1) + (targetMax * 0.9 * progress) : 0);

	const margin = { top: 40, right: 40, bottom: 80, left: 80 };
	const width = 800 - margin.left - margin.right;
	const height = 500 - margin.bottom - margin.top;

	// Scales - y scale now uses currentMax instead of overall max
	const xScale = $derived(
		scaleBand()
			.domain(data.map(d => d.year))
			.range([0, width])
			.padding(0.2)
	);

	const yScale = $derived(
		scaleLinear()
			.domain([0, currentMax])
			.nice()
			.range([height, 0])
	);

	// Format large numbers with commas
	function formatNumber(num) {
		return new Intl.NumberFormat().format(num);
	}

	// Get gold color for all bars to simulate light bulbs
	function getBarColor(lightType) {
		return '#ffd700'; // Gold color
	}
</script>

<div class="chart-container" style="background: transparent; transition: background 0.3s ease;">
	<h3 style="color: {headerColor}; transition: color 0.3s ease;">Labor Price Over Time (in Seconds)</h3>
	<svg {width} {height} viewBox="0 0 {width + margin.left + margin.right} {height + margin.top + margin.bottom}">
		<!-- Define glow filter -->
		<defs>
			<filter id="glow" x="-50%" y="-50%" width="200%" height="200%">
				<feGaussianBlur stdDeviation="3" result="coloredBlur"/>
				<feMerge> 
					<feMergeNode in="coloredBlur"/>
					<feMergeNode in="SourceGraphic"/>
				</feMerge>
			</filter>
		</defs>
		<g transform="translate({margin.left}, {margin.top})">
			<!-- Y-axis -->
			<g class="axis y-axis">
				<line x1="0" y1="0" x2="0" y2={height} stroke={textColor} stroke-width="1" style="transition: stroke 0.3s ease;"/>
				{#each yScale.ticks(8) as tick, i}
					<g 
						transform="translate(0, {yScale(tick)})"
						class="y-tick-group"
						style="transition: transform 1.5s ease-out;"
					>
						<line 
							x1="-5" 
							y1="0" 
							x2="0" 
							y2="0" 
							stroke={textColor} 
							stroke-width="1"
							class="y-tick-line"
							style="transition: stroke 0.3s ease;"
						/>
						<text 
							x="-10" 
							y="0" 
							dy="0.35em" 
							text-anchor="end" 
							class="tick-label"
							fill={textColor}
							style="transition: fill 0.3s ease;"
						>
							{formatNumber(tick)}
						</text>
					</g>
				{/each}
				<text 
					x={-50} 
					y={height/2} 
					text-anchor="middle" 
					transform="rotate(-90, {-50}, {height/2})"
					class="axis-label"
					fill={textColor}
					style="transition: fill 0.3s ease;"
				>
					Labor Price (Seconds)
				</text>
			</g>
			
			<!-- X-axis -->
			<g class="axis x-axis" transform="translate(0, {height})">
				<line x1="0" y1="0" x2={width} y2="0" stroke={textColor} stroke-width="1" style="transition: stroke 0.3s ease;"/>
				{#each data as d}
					<g transform="translate({xScale(d.year) + xScale.bandwidth()/2}, 0)">
						<line x1="0" y1="0" x2="0" y2="5" stroke={textColor} stroke-width="1" style="transition: stroke 0.3s ease;"/>
						<text x="0" y="20" text-anchor="middle" class="tick-label" fill={textColor} style="transition: fill 0.3s ease;">
							{d.year}
						</text>
						<text x="0" y="40" text-anchor="middle" class="light-type-label" fill={textColor} style="transition: fill 0.3s ease; opacity: 0.7;">
							{d.light}
						</text>
					</g>
				{/each}
				<text 
					x={width/2} 
					y="70" 
					text-anchor="middle"
					class="axis-label"
					fill={textColor}
					style="transition: fill 0.3s ease;"
				>
					Year / Technology
				</text>
			</g>
			
			<!-- Bars -->
			{#each visibleData as d, i}
				<rect 
					x={xScale(d.year)}
					y={yScale(d.labor_price_seconds) + (height - yScale(d.labor_price_seconds)) * (1 - progress)}
					width={xScale.bandwidth()}
					height={(height - yScale(d.labor_price_seconds)) * progress}
					fill={getBarColor(d.light)}
					class="bar"
					filter="url(#glow)"
					style="transition: y 0.3s ease, height 0.3s ease; drop-shadow: 0 0 10px #ffd700;"
				/>
				<!-- Value labels on bars - only show when timer completes -->
				<text 
					x={xScale(d.year) + xScale.bandwidth()/2}
					y={yScale(d.labor_price_seconds) - 10 + (height - yScale(d.labor_price_seconds)) * (1 - progress)}
					text-anchor="middle"
					class="bar-label"
					fill={textColor}
					style="opacity: {progress >= 1 ? 1 : 0}; transition: y 0.3s ease, fill 0.3s ease, opacity 0.3s ease;"
				>
					{formatNumber(d.labor_price_seconds)}s
				</text>
			{/each}
		</g>
	</svg>
</div>

<style>
	.chart-container {
		padding: 2rem;
		border-radius: 8px;
		max-width: 900px;
		margin: 0 auto;
	}
	
	.chart-container h3 {
		text-align: center;
		margin-bottom: 1rem;
		font-size: 1.5rem;
	}
	
	.axis-label {
		font-size: 14px;
		font-weight: 600;
	}
	
	.tick-label {
		font-size: 12px;
	}
	
	.y-tick-group {
		transition: transform 1.5s ease-out;
	}
	
	.light-type-label {
		font-size: 10px;
		font-style: italic;
	}
	
	.bar {
		transform-origin: bottom;
		filter: drop-shadow(0 0 8px rgba(255, 215, 0, 0.6)) 
		        drop-shadow(0 0 15px rgba(255, 215, 0, 0.4))
		        drop-shadow(0 0 25px rgba(255, 215, 0, 0.2));
	}
	
	.bar-label {
		font-size: 12px;
		font-weight: 600;
	}
</style>