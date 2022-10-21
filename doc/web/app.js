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

    $.ajax({
        url: '/start.json',
        dataType: 'json',
        method: 'GET',
        cache: false
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
        headers: { 'frequency': value['frequency'], 'duty_a': value['duty_a'], 'duty_b': value['duty_b'] }
    });
}


const form_carrier = document.querySelector('#form_carrier');
form_carrier.addEventListener('submit', handlecarrier);

const form = document.querySelector('#form_modulation');
form.addEventListener('submit', handlemodulation);