/**
 * Add gobals here
 */
var seconds = null;
var otaTimerVar = null;



/**
 * Handles the firmware update.
 */
function updateFirmware() {
    // Form Data
    var formData = new FormData();
    var fileSelect = document.getElementById("selected_file");

    if (fileSelect.files && fileSelect.files.length == 1) {
        var file = fileSelect.files[0];
        formData.set("file", file, file.name);
        document.getElementById("ota_update_status").innerHTML = "Uploading " + file.name + ", Firmware Update in Progress...";

        // Http Request
        var request = new XMLHttpRequest();

        request.upload.addEventListener("progress", updateProgress);
        request.open('POST', "/OTAupdate");
        request.responseType = "blob";
        request.send(formData);
    } else {
        window.alert('Select A File First')
    }
}

/**
 * Progress on transfers from the server to the client (downloads).
 */
function updateProgress(oEvent) {
    if (oEvent.lengthComputable) {
        getUpdateStatus();
    } else {
        window.alert('total size is unknown')
    }
}

/**
 * Posts the firmware udpate status.
 */
function getUpdateStatus() {
    var xhr = new XMLHttpRequest();
    var requestURL = "/OTAstatus";
    xhr.open('POST', requestURL, false);
    xhr.send('ota_update_status');

    if (xhr.readyState == 4 && xhr.status == 200) {
        var response = JSON.parse(xhr.responseText); // Return JSON from string in server

        document.getElementById("latest_firmware").innerHTML = response.compile_date + " - " + response.compile_time

        // If flashing was complete it will return a 1, else -1
        // A return of 0 is just for information on the Latest Firmware request
        if (response.ota_update_status == 1) {
            // Set the countdown timer time
            seconds = 10;

            // Start the countdown timer
            otaTimerVar = setInterval(otaRebootTimer, 1000);
        } else if (response.ota_update_status == -1) {
            document.getElementById("ota_update_status").innerHTML = "!!! Upload Error !!!";
        }
    }
}

/**
 * Displays the reboot countdown.
 */
function otaRebootTimer() {
    --seconds;
    document.getElementById("ota_update_status").innerHTML = "OTA Firmware Update Complete. This page will close shortly, Rebooting in: " + seconds;

    if (seconds == 0) {
        clearTimeout(otaTimerVar);
        window.location.reload();
    }
}


/**
 * Seend request for open door event.
 */
function start_button() {

    let frequency = document.getElementById('frequency').value;
    let ppw = document.getElementById('ppw').value;
    let npw = document.getElementById('npw').value;

    let T1 = document.getElementById('t1').value;
    let T2 = document.getElementById('t2').value;
    let T3 = document.getElementById('t3').value;

    let intensity = document.getElementById('intensity').value;

    let time_treatment = document.getElementById('time_treatment').value;

    $.ajax({
        url: '/start.json',
        dataType: 'json',
        method: 'POST',
        cache: false,
        headers: { 'frequency': frequency, 'ppw': ppw, 'npw': npw, 'T1': T1, 'T2': T2, 'T3': T3, 'intensity': intensity, 'time_treatment': time_treatment }
    });
}

function stop_button() {

    $.ajax({
        url: '/stop.json',
        dataType: 'json',
        method: 'GET',
        cache: false
    });


}

function handlemodulation(event) {
    event.preventDefault();

    const data = new FormData(event.target);

    const value = Object.fromEntries(data.entries());

    console.log({ value });

    $.ajax({
        url: '/handlemodulation.json',
        dataType: 'json',
        method: 'POST',
        cache: false,
        headers: { 't1': value['t1'], 't2': value['t2'], 't3': value['t3'] }
    });
}

function handlecarrier(event) {
    event.preventDefault();

    const data = new FormData(event.target);

    const value = Object.fromEntries(data.entries());

    console.log({ value });

    $.ajax({
        url: '/handlecarrier.json',
        dataType: 'json',
        method: 'POST',
        cache: false,
        headers: { 'frequency': value['frequency'], 'ppw': value['ppw'], 'npw': value['npw'] }
    });
}


function updateTextInput_ppw(val) {
    document.getElementById('value_ppw').innerHTML = '<b>' + val + '%' + '</b>';
}

function updateTextInput_npw(val) {
    document.getElementById('value_npw').innerHTML = '<b>' + val + '%' + '</b>';
}

function updateIntensityRange(val) {
    document.getElementById('intensity_value').innerHTML = '<b>' + val + '%' + '</b>';
}