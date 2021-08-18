var timeout = 1000;

function update_content(data) {
    document.getElementById("container").innerHTML = data;
    setTimeout(start, timeout);
}

function update() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
           update_content(xhttp.responseText);
        }
    };

    xhttp.open("GET", "update", true);
    xhttp.send();
}

function start() {
    update();
}
