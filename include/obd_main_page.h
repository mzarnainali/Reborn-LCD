const char P_OBD_MAIN_PAGE[] PROGMEM = R"rawliteral(
 <!DOCTYPE HTML>
<html>
<head>
  <title>ESP32 CANViewer</title>
  <meta charset="utf-8"/>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    #log { white-space: pre-wrap; padding: 10px; height: 70vh; overflow-y: scroll; }
  </style>
</head>
<body onload="initWebSocket();">
  <center>
    <h1 style="margin-bottom: 0px;">ESP32 CANViewer</h1>
  <div>
    <div id="statusdiv">
      <span id="connectiondisplay"></span><br>
      <button onclick="initWebSocket()">Connect</button> | 
      <button onclick="disconnectWebSocket()">Disconnect</button> | 
      <button onclick="downloadDivAsText()">Download as .txt</button> | 
      <button onclick="addLineBreak()">Add Line Break</button> | 
      <button onclick="clearDiv()">Clear</button>
    </div>
      </center>
    <div id="log"></div>
  </div>

<script>
  var gateway = `ws://${window.location.hostname}/ws`;
  //var gateway = 'ws://192.168.178.120:1337/';
  var websocket;
  var reconnectTimer = 0;
  const logDiv = document.getElementById("log");

  function initWebSocket() {

  if (websocket && websocket.readyState === WebSocket.OPEN) return;

    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage;
    autoConnect = true;
  }

  function disconnectWebSocket() {
  if (websocket && websocket.readyState === WebSocket.OPEN) {
    websocket.close();
    autoConnect = false;
    clearTimeout(reconnectTimer);
    log("👋 Disconnect requested");
  }
}

websocket.onerror = (e) => {
  console.warn("WebSocket error:", e);
  websocket.close();  // <- this causes fast disconnect
};

  
  function onOpen(event) {
    console.log('Connection opened');
    document.getElementById('connectiondisplay').innerHTML = "CONNECTED";
  }
  
  var autoConnect = true;
  function onClose(event) {
    console.log('Connection closed');
    document.getElementById('connectiondisplay').innerHTML = "DISCONNECTED";
    if(autoConnect){
    reconnectTimer = setTimeout(initWebSocket, 2000);
   }
  }
  
  function onMessage(event) {
      logDiv.textContent += event.data;
      logDiv.scrollTop = logDiv.scrollHeight; // Auto-scroll
  }

 function downloadDivAsText() {
  const logDiv = document.getElementById("log");
  let content = logDiv.innerText;

  // Generate timestamp: YYYY-MM-DD_HH-MM-SS
  const now = new Date();
  const timestamp = now.getFullYear() + "-" +
    String(now.getMonth() + 1).padStart(2, '0') + "-" +
    String(now.getDate()).padStart(2, '0') + "_" +
    String(now.getHours()).padStart(2, '0') + "-" +
    String(now.getMinutes()).padStart(2, '0') + "-" +
    String(now.getSeconds()).padStart(2, '0');

  const filename = `log_${timestamp}.txt`;

  const blob = new Blob([content], { type: 'text/plain' });
  const url = URL.createObjectURL(blob);

  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  a.click();

  URL.revokeObjectURL(url); // Clean up
}

function addLineBreak(){
logDiv.textContent += "******************************************************\n\n";
logDiv.scrollTop = logDiv.scrollHeight; // Auto-scroll
}

function clearDiv(){
logDiv.textContent = "";
logDiv.scrollTop = logDiv.scrollHeight; // Auto-scroll
}

setInterval(() => {
  if (websocket && websocket.readyState === WebSocket.OPEN) {
    websocket.send("ping");
  }
}, 20000); // every 20 seconds
</script>
</body>
</html>
        )rawliteral";