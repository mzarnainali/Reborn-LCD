// HTML Interface stored in PROGMEM to save RAM
const char FUEL_DOT_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Civic Reborn - Fuel Calibration</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #121212; color: #e0e0e0; margin: 0; padding: 20px; }
        .container { max-width: 1100px; margin: 0 auto; }
        h2 { color: #00e676; border-bottom: 2px solid #333; padding-bottom: 10px; }
        .card { background: #1e1e1e; padding: 20px; border-radius: 10px; margin-bottom: 20px; box-shadow: 0 4px 10px rgba(0,0,0,0.5); }
        .grid-header, .grid-row { display: grid; grid-template-columns: 80px repeat(6, 1fr); gap: 8px; align-items: center; text-align: center; }
        .grid-header { font-weight: bold; padding-bottom: 10px; color: #888; border-bottom: 1px solid #333; }
        .grid-row { padding: 6px 0; border-bottom: 1px solid #2a2a2a; }
        input[type="number"] { width: 90%; padding: 6px; border: 1px solid #00e676; background: #2a2a2a; color: #fff; border-radius: 4px; text-align: center; font-weight: bold; }
        input[readonly] { background: #181818; border: 1px solid #333; color: #777; }
        .btn-save { background: #00e676; color: #000; font-weight: bold; padding: 12px 24px; border: none; border-radius: 6px; cursor: pointer; float: right; margin-top: 15px; font-size: 16px; }
        .btn-save:hover { background: #00c853; }
        .total-box { font-size: 18px; font-weight: bold; color: #00e676; margin-top: 10px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="card">
            <h2>Honda Civic Fuel Segment Calibrations (50L Tank)</h2>
            <div id="grid">
                <!-- Data dynamically inserted here -->
            </div>
            <div style="overflow: hidden; margin-top: 15px;">
                <span class="total-box" id="p0Total">Profile 0 Total: -- L</span>
                <button class="btn-save" onclick="saveProfile0()">Save Profile 0</button>
            </div>
        </div>
    </div>

<script>
async function loadProfiles() {
    try {
        let res = await fetch('/api/fuel-profiles');
        let data = await res.json();
        renderGrid(data);
    } catch(e) {
        alert("Failed to load profiles from ESP32");
    }
}

function renderGrid(data) {
    let container = document.getElementById('grid');
    let html = `<div class="grid-header">
        <div>Dot #</div>
        <div style="color: #00e676;">Active (P0)</div>
        <div>Ref P1</div>
        <div>Ref P2</div>
        <div>Ref P3</div>
        <div>Ref P4</div>
        <div>Ref P5</div>
    </div>`;

    for (let i = 0; i < 20; i++) {
        // Math.abs() converts negative numbers to positive for display
        let p0Val = Math.abs(parseFloat(data.profiles[0][i]) || 0).toFixed(2);

        html += `<div class="grid-row">
            <div><strong>Dot ${i+1}</strong></div>
            <div><input type="number" step="0.01" id="p0_${i}" value="${p0Val}" oninput="calcTotal()"></div>`;
        
        for (let p = 1; p <= 5; p++) {
            // Display reference profiles as positive numbers too
            let refVal = Math.abs(parseFloat(data.profiles[p][i]) || 0).toFixed(2);
            html += `<div><input type="number" value="${refVal}" readonly></div>`;
        }
        html += `</div>`;
    }

    container.innerHTML = html;
    calcTotal();
}

function calcTotal() {
    let sum = 0;
    for (let i = 0; i < 20; i++) {
        let val = parseFloat(document.getElementById(`p0_${i}`).value) || 0;
        sum += val;
    }
    document.getElementById('p0Total').innerText = `Profile 0 Total: ${sum.toFixed(1)} L / 50.0 L`;
}

async function saveProfile0() {
    let p0Array = [];
    for (let i = 0; i < 20; i++) {
        let val = parseFloat(document.getElementById(`p0_${i}`).value) || 0;
        p0Array.push(val);
    }

    let res = await fetch('/api/save-profile0', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ profile0: p0Array })
    });

    if (res.ok) {
        alert("Profile 0 saved successfully!");
    } else {
        alert("Error saving Profile 0.");
    }
}

window.onload = loadProfiles;
</script>
</body>
</html>
)rawliteral";