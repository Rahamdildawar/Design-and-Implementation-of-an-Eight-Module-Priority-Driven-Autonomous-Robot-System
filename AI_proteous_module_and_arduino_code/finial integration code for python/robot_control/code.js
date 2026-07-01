// Example: Sending scenario changes to backend
function changeScenario(modeName) {
    fetch('/api/set_scenario', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ scenario: modeName })
    })
    .then(res => res.json())
    .then(data => console.log("Scenario changed:", data));
}

// Example: Polling the engine telemetry state
setInterval(() => {
    fetch('/api/state')
    .then(res => res.json())
    .then(state => {
        document.getElementById('fps-counter').innerText = state.fps;
        document.getElementById('active-byte').innerText = state.active_byte;
    });
}, 200); // Polls every 200ms