<script>
	import { scaleLinear, scaleBand } from "d3-scale";
	import { max } from "d3-array";
	
	let { data = [], step = 0, progress = 1, backgroundColor = '#ffffff', textColor = '#000000', headerColor = '#333333' } = $props();
	
	console.log('BarChart rendering with:', { 
		dataLength: data?.length, 
		step, 
		progress, 
		dataStep0: data?.[0],
		timestamp: Date.now()
	});
	
	// Show data based on timer progress and step
	const visibleData = $derived.by(() => {
		console.log('visibleData calculating with:', { step, dataLength: data.length });
		const result = data.slice(0, step + 1);
		console.log('visibleData result:', { visibleLength: result.length, result });
		return result;
	});
	
	// Get the value to use for scaling based on current step's time unit
	function getScaleValue(dataItem, currentStep) {
		const timeDisplay = getTimeDisplay(dataItem, currentStep);
		return timeDisplay.value;
	}

	// Calculate the scale that should be used based on timer progress
	const currentMax = $derived.by(() => {
		console.log('currentMax calculating with step:', step, 'progress:', progress);
		if (progress === 0) {
			// Timer not running - use max of data up to PREVIOUS step only
			// This keeps scale stable when switching steps
			const dataUpToPrevious = step > 0 ? data.slice(0, step) : [];
			if (dataUpToPrevious.length > 0) {
				const result = max(dataUpToPrevious, d => getScaleValue(d, step)) || 0;
				console.log('currentMax (no progress, has previous):', result);
				return result;
			} else {
				// No previous steps (first timer step), use a reasonable default
				const currentStepData = data[step];
				const result = currentStepData ? getScaleValue(currentStepData, step) : 100;
				console.log('currentMax (no progress, no previous):', result);
				return result;
			}
		} else {
			// Timer is running - interpolate scale between previous max and final max
			const currentStepData = data[step];
			const previousStepsMax = step > 0 ? max(data.slice(0, step), d => getScaleValue(d, step)) || 0 : 0;
			const currentStepMax = currentStepData ? getScaleValue(currentStepData, step) : 0;
			const finalMax = Math.max(previousStepsMax, currentStepMax);
			
			// Start with previous max, gradually scale up to final max as timer progresses
			// For first step, start with a small scale and grow to current step max
			const startMax = previousStepsMax > 0 ? previousStepsMax : currentStepMax * 0.1;
			const result = startMax + (finalMax - startMax) * progress;
			console.log('currentMax (with progress):', result);
			return result;
		}
	});

	// Responsive dimensions based on viewport - conservative sizing for laptops
	const getResponsiveDimensions = () => {
		const vw = Math.max(document.documentElement.clientWidth || 0, window.innerWidth || 0);
		const vh = Math.max(document.documentElement.clientHeight || 0, window.innerHeight || 0);
		
		// Very conservative dimensions to ensure everything fits
		// Account for header (~12vh), timer (~12vh), padding (~6vh), leaving ~70vh available
		// Use only ~50vh for chart to be safe
		const baseWidth = Math.min(Math.max(vw * 0.7, 450), 900);
		const baseHeight = Math.min(Math.max(vh * 0.35, 250), 450);
		
		return {
			margin: { 
				top: Math.max(25, vh * 0.025), 
				right: Math.max(25, vw * 0.02), 
				bottom: Math.max(50, vh * 0.05), 
				left: Math.max(80, vw * 0.06) 
			},
			width: baseWidth,
			height: baseHeight
		};
	};

	let dimensions = $state(getResponsiveDimensions());
	
	// Update dimensions on window resize
	if (typeof window !== 'undefined') {
		window.addEventListener('resize', () => {
			dimensions = getResponsiveDimensions();
		});
	}

	// Scales - y scale now uses currentMax instead of overall max
	const xScale = $derived(
		scaleBand()
			.domain(data.map(d => d.year))
			.range([0, dimensions.width - dimensions.margin.left - dimensions.margin.right])
			.padding(0.2)
	);

	const yScale = $derived(
		scaleLinear()
			.domain([0, currentMax])
			.nice()
			.range([dimensions.height - dimensions.margin.top - dimensions.margin.bottom, 0])
	);

	// Format large numbers with commas
	function formatNumber(num) {
		return new Intl.NumberFormat().format(num);
	}
	
	// Get the appropriate time unit and value based on step
	function getTimeDisplay(laborData, currentStep) {
		const actualStep = currentStep + 2; // Convert from 0-based to actual step number
		
		if (actualStep <= 3) {
			// Steps 2-3: Use seconds
			return {
				value: laborData.labor_price_seconds,
				unit: 'second',
				unitShort: 's'
			};
		} else if (actualStep === 4) {
			// Step 4 (1879): Use minutes
			return {
				value: laborData.labor_price_minutes,
				unit: 'minute',
				unitShort: 'min'
			};
		} else {
			// Steps 5-7: Use hours
			return {
				value: laborData.labor_price_hours,
				unit: 'hour',
				unitShort: 'h'
			};
		}
	}

	// Get gold color for all bars to simulate light bulbs
	function getBarColor(lightType) {
		return '#ffd700'; // Gold color
	}
	
	// Clean up light type labels by removing "lamp" where it appears
	function cleanLightLabel(lightType) {
		return lightType.replace(/\s*lamp\s*/gi, '').trim();
	}
</script>

<div class="chart-container" style="background: transparent; transition: background 0.3s ease;">
	<h3 style="color: {headerColor}; transition: color 0.3s ease; font-size: clamp(1.2rem, 3.2vw, 2.4rem); margin-bottom: 1vh;">Work time needed to afford an hour of light</h3>
	<svg 
		width={dimensions.width} 
		height={dimensions.height} 
		viewBox="0 0 {dimensions.width} {dimensions.height}"
		style="max-width: 100%; height: auto;"
	>
		<g transform="translate({dimensions.margin.left}, {dimensions.margin.top})">
			<!-- Y-axis -->
			<g class="axis y-axis">
				<line x1="0" y1="0" x2="0" y2={dimensions.height - dimensions.margin.top - dimensions.margin.bottom} stroke={textColor} stroke-width="1" style="transition: stroke 0.3s ease;"/>
				{#each yScale.ticks(8) as tick, i}
					{@const timeDisplay = step >= 0 && data[0] ? getTimeDisplay(data[0], step) : {unit: 'second'}}
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
							style="transition: fill 0.3s ease; font-size: clamp(0.7rem, 1.8vw, 1.2rem);"
						>
							{formatNumber(tick)}{i === yScale.ticks(8).length - 1 ? ` ${timeDisplay.unit}s` : ''}
						</text>
					</g>
				{/each}
			</g>
			
			<!-- X-axis -->
			<g class="axis x-axis" transform="translate(0, {dimensions.height - dimensions.margin.top - dimensions.margin.bottom})">
				<line x1="0" y1="0" x2={dimensions.width - dimensions.margin.left - dimensions.margin.right} y2="0" stroke={textColor} stroke-width="1" style="transition: stroke 0.3s ease;"/>
				{#each visibleData as d, i}
					{#if i <= step}
						<g transform="translate({xScale(d.year) + xScale.bandwidth()/2}, 0)">
							<line x1="0" y1="0" x2="0" y2="5" stroke={textColor} stroke-width="1" style="transition: stroke 0.3s ease;"/>
							<text x="0" y="20" text-anchor="middle" class="tick-label" fill={textColor} style="transition: fill 0.3s ease; font-size: clamp(0.7rem, 1.8vw, 1.2rem);">
								{d.year}
							</text>
							<text x="0" y="40" text-anchor="middle" class="light-type-label" fill={textColor} style="transition: fill 0.3s ease; opacity: 0.7; font-size: clamp(0.6rem, 1.5vw, 1rem);">
								{cleanLightLabel(d.light)}
							</text>
						</g>
					{/if}
				{/each}
				<text 
					x={(dimensions.width - dimensions.margin.left - dimensions.margin.right)/2} 
					y="70" 
					text-anchor="middle"
					class="axis-label"
					fill={textColor}
					style="transition: fill 0.3s ease; font-size: clamp(0.8rem, 2vw, 1.5rem);"
				>
					Year / Technology
				</text>
			</g>
			
			<!-- Bars -->
			{console.log('About to loop. step:', step, 'data.length:', data.length, 'visibleData length:', visibleData.length)}
			{#each data.slice(0, step + 1) as d, i}
				{@const debugInfo = `Bar ${i}: step=${step}, progress=${progress}, currentMax=${currentMax}, value=${d.labor_price_seconds}`}
				{console.log(debugInfo)}
				{#if i <= step}
					{@const isCurrentStep = i === step}
					{@const barProgress = isCurrentStep ? progress : 1}
					{@const timeDisplay = getTimeDisplay(d, step)}
					{@const barX = xScale(d.year)}
					{@const barY = yScale(timeDisplay.value)}
					{@const barWidth = xScale.bandwidth()}
					{@const barHeight = (dimensions.height - dimensions.margin.top - dimensions.margin.bottom) - yScale(timeDisplay.value)}
					{@const actualBarHeight = barHeight * barProgress}
					{@const actualBarY = barY + barHeight * (1 - barProgress)}
					{console.log(`Calculated: x=${barX}, y=${actualBarY}, width=${barWidth}, height=${actualBarHeight}, barProgress=${barProgress}, currentMaxValue=${currentMax}`)}
					{#if barProgress > 0 && actualBarHeight > 0}
						<rect 
							x={barX}
							y={actualBarY}
							width={barWidth}
							height={actualBarHeight}
							fill={getBarColor(d.light)}
							class="bar"
							style="transition: y 0.3s ease, height 0.3s ease;"
						/>
						<!-- Value labels on bars - only show when bar is complete -->
						<text 
							x={xScale(d.year) + xScale.bandwidth()/2}
							y={yScale(timeDisplay.value) - 10 + ((dimensions.height - dimensions.margin.top - dimensions.margin.bottom) - yScale(timeDisplay.value)) * (1 - barProgress)}
							text-anchor="middle"
							class="bar-label"
							fill={textColor}
							style="opacity: {barProgress >= 1 ? 1 : 0}; transition: y 0.3s ease, fill 0.3s ease, opacity 0.3s ease; font-size: clamp(0.6rem, 1.5vw, 1rem);"
						>
							{formatNumber(timeDisplay.value)}{timeDisplay.unitShort}
						</text>
					{/if}
				{/if}
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
	}
	
	.bar-label {
		font-size: 12px;
		font-weight: 600;
	}
</style>