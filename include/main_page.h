const char P_MAIN_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>CAN ID Config</title>

  </head>
  <body>
    <h1>TPMS ID Configuration</h1>

    <label for="id_fl">ID_FL:
      <select id="id_fl" name="id_fl">
      <option value="">Select</option>
        <option value="0x1F9E17UL">0x1F9E17UL</option>
        <option value="0x1D2C01UL">0x1D2C01UL</option>
        <option value="0x1FAA50UL">0x1FAA50UL</option>
        <option value="0x1D500EUL">0x1D500EUL</option>
      </select>
    </label>

    <label for="id_fr">ID_FR:
  <select id="id_fr" name="id_fr">
  <option value="">Select</option>
    <option value="0x1F9E17UL">0x1F9E17UL</option>
    <option value="0x1D2C01UL">0x1D2C01UL</option>
    <option value="0x1FAA50UL">0x1FAA50UL</option>
    <option value="0x1D500EUL">0x1D500EUL</option>
  </select>
</label>
<br>
<label for="id_rl">ID_RL:
  <select id="id_rl" name="id_rl">
  <option value="">Select</option>
    <option value="0x1F9E17UL">0x1F9E17UL</option>
    <option value="0x1D2C01UL">0x1D2C01UL</option>
    <option value="0x1FAA50UL">0x1FAA50UL</option>
    <option value="0x1D500EUL">0x1D500EUL</option>
  </select>
</label>
<label for="id_rr">ID_RR:
  <select id="id_rr" name="id_rr">
  <option value="">Select</option>
    <option value="0x1F9E17UL">0x1F9E17UL</option>
    <option value="0x1D2C01UL">0x1D2C01UL</option>
    <option value="0x1FAA50UL">0x1FAA50UL</option>
    <option value="0x1D500EUL">0x1D500EUL</option>
  </select>
</label>

    
    <br><br>
    <button onclick="saveIDs()">Save</button>
    <button onclick=updateDefault()> Set Default</button>
	
	</hr>
	<h1>Time Configuration</h1>
	<form action='/setTime' method='POST'>
                                   Date (YYYY-MM-DD): <input type='text' name='date'><br>
                                   "Time (HH:MM:SS): <input type='text' name='time'><br>
                                   <input type='submit' value='Set Time'>
                                   </form>
  
      <script>
      async function loadIDs() {
        const res = await fetch('/get_ids');
        const ids = await res.json();
        // Format: '0x' + uppercase hex + 'UL'
  document.getElementById('id_fl').value = '0x' + ids.id_fl.toString(16).toUpperCase() + 'UL';
  document.getElementById('id_fr').value = '0x' + ids.id_fr.toString(16).toUpperCase() + 'UL';
  document.getElementById('id_rl').value = '0x' + ids.id_rl.toString(16).toUpperCase() + 'UL';
  document.getElementById('id_rr').value = '0x' + ids.id_rr.toString(16).toUpperCase() + 'UL';
      }

      async function saveIDs() {
        const ids = {
          id_fl: document.getElementById('id_fl').value,
          id_fr: document.getElementById('id_fr').value,
          id_rl: document.getElementById('id_rl').value,
          id_rr: document.getElementById('id_rr').value
        };
        await fetch('/set_ids', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify(ids)
        });
        alert('Saved!');
      }
      
      	function updateDefault() {
			var xhttp = new XMLHttpRequest();
			xhttp.onreadystatechange = function () {
				if (this.readyState == 4 && this.status == 200) {
		      alert('Saved!');
				}
			}
			xhttp.open("GET", '/update_default', true);
			xhttp.send();
		}

      window.onload = loadIDs;
    </script>
                                   </body>
</html>
)rawliteral";