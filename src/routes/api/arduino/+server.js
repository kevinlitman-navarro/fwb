import { json } from '@sveltejs/kit';

// Store for broadcasting to connected clients
const clients = new Set();

/** @type {import('./$types').RequestHandler} */
export async function POST({ request }) {
	try {
		const data = await request.json();
		
		// Validate the data
		if (!data.type) {
			return json({ error: 'Missing type field' }, { status: 400 });
		}
		
		// Log for debugging
		console.log('Arduino data received:', data);
		
		// Broadcast to all connected clients via Server-Sent Events
		const message = `data: ${JSON.stringify(data)}\n\n`;
		clients.forEach(client => {
			try {
				client.write(message);
			} catch (error) {
				// Remove dead connections
				clients.delete(client);
			}
		});
		
		return json({ success: true });
		
	} catch (error) {
		console.error('Error processing Arduino data:', error);
		return json({ error: 'Invalid JSON' }, { status: 400 });
	}
}

/** @type {import('./$types').RequestHandler} */
export async function GET({ request }) {
	// Server-Sent Events endpoint for the web app to receive Arduino data
	const stream = new ReadableStream({
		start(controller) {
			const headers = new Headers({
				'Content-Type': 'text/event-stream',
				'Cache-Control': 'no-cache',
				'Connection': 'keep-alive',
				'Access-Control-Allow-Origin': '*',
				'Access-Control-Allow-Headers': 'Cache-Control'
			});
			
			// Send initial connection message
			const initialMessage = `data: ${JSON.stringify({ type: 'connected' })}\n\n`;
			controller.enqueue(new TextEncoder().encode(initialMessage));
			
			// Store this client
			const client = {
				write: (data) => {
					controller.enqueue(new TextEncoder().encode(data));
				}
			};
			clients.add(client);
			
			// Clean up on disconnect
			request.signal?.addEventListener('abort', () => {
				clients.delete(client);
				controller.close();
			});
		}
	});
	
	return new Response(stream, {
		headers: {
			'Content-Type': 'text/event-stream',
			'Cache-Control': 'no-cache',
			'Connection': 'keep-alive',
			'Access-Control-Allow-Origin': '*',
			'Access-Control-Allow-Headers': 'Cache-Control'
		}
	});
}