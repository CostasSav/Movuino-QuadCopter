//Tab system ------------------------------------------------------------------------
var currentTab;

function openTab(evt, tabName) {
  // Save tab for other uses (ex: onMessage function)
  currentTab = tabName;
  // Declare all variables
  var i, tabcontent, tablinks;

  // Get all elements with class="tabcontent" and hide them
  tabcontent = document.getElementsByClassName("tabcontent");
  for (i = 0; i < tabcontent.length; i++) {
    tabcontent[i].style.display = "none";
  }

  // Get all elements with class="tablinks" and remove the class "active"
  tablinks = document.getElementsByClassName("tablinks");
  for (i = 0; i < tablinks.length; i++) {
    tablinks[i].className = tablinks[i].className.replace(" active", "");
  }

  // Show the current tab, and add an "active" class to the button that opened the tab
  document.getElementById(tabName).style.display = "block";
  evt.currentTarget.className += " active";
}


//WebSocket --------------------------------------------------------------------------

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onload);

function onload(event) {
  console.log(gateway);
  initWebSocket();
}

function getValues(){
  websocket.send("getValues");
}

function initWebSocket() {
  console.log('Trying to open a WebSocket connection…');
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
}

function onOpen(event) {
  console.log('Connection opened');
  getValues();
  websocket.send("ko");
}

function onClose(event) {
  console.log('Connection closed');
  setTimeout(initWebSocket, 2000);
}


//Flight Tab ------------------------------------------------------------------------------------
var powerState = "off";
var offbutton = document.getElementById("offbutton");
offbutton.onclick = function () {
  if (powerState == "off") {
    powerState = "on";
    offbutton.innerHTML = "TURN OFF";
    websocket.send("ho");
  } else {
    powerState = "off";
    offbutton.innerHTML = "TURN ON";
    websocket.send("ko");
  }
}


function mouseUp() { //resets command and makes drone hover
  websocket.send("ho");
}

//send controller command to the Movuino
var left1 = document.getElementById("left1");
var up1 = document.getElementById("up1");
var right1 = document.getElementById("right1");
var down1 = document.getElementById("down1");
function straightLeft() {
  websocket.send("ll");
}
function straightForward() {
  websocket.send("lu");
}
function straightRight() {
  websocket.send("lr");
}
function straightReverse() {
  websocket.send("ld");
}


var left2 = document.getElementById("left2");
var up2 = document.getElementById("up2");
var right2 = document.getElementById("right2");
var down2 = document.getElementById("down2");
function rotateLeft() {
  websocket.send("rl");
}
function altitudeGain() {
  websocket.send("ru");
}
function rotateRight() {
  websocket.send("rr");
}
function altitudeLoss() {
  websocket.send("rd");
}


function onMessage(event) {
  var myObj = JSON.parse(event.data);
  var keys = Object.keys(myObj);
  if (currentTab == 'Testing') { //Testing Tab -------------------------------------
    console.log(event.data);

    if (keys.length != 9) { //ignore message if it is mpu data
      for (var i = 0; i < keys.length; i++) {
        var key = keys[i];
        document.getElementById(key).innerHTML = myObj[key];
        document.getElementById("slider" + (i + 1).toString()).value = myObj[key];
      }
    }

  }else if (currentTab == 'Data') { //Data Tab --------------------------------------
    console.log(event.data)
   
    for (var i = 0; i < keys.length; i++) {
      var key = keys[i];
      document.getElementById("mpu" + (i + 1).toString()).innerHTML = myObj[key];

      //for continuous stream of data:
      //document.getElementById("mpu" + (i + 1).toString()).innerHTML = document.getElementById("mpu" + (i + 1).toString()).innerHTML + " " + myObj[key];
    }
  }
}


//1st motor
var slider1 = document.getElementById("slider1");
var motor1 = document.getElementById("motor1");
motor1.innerHTML = slider1.value; // Display the default slider value
slider1.oninput = function() {// Update the current slider value (each time you drag the slider handle)
  motor1.innerHTML = slider1.value;
  console.log(slider1.value);
  websocket.send("1"+"s"+slider1.value.toString());
}

//2nd motor
var slider2 = document.getElementById("slider2");
var motor2 = document.getElementById("motor2");
motor2.innerHTML = slider2.value; // Display the default slider value
slider2.oninput = function() {// Update the current slider value (each time you drag the slider handle)
  motor2.innerHTML = slider2.value;
  console.log(slider2.value);
  websocket.send("2"+"s"+slider2.value.toString());
}

//3rd motor
var slider3 = document.getElementById("slider3");
var motor3 = document.getElementById("motor3");
motor3.innerHTML = slider3.value; // Display the default slider value
slider3.oninput = function() {// Update the current slider value (each time you drag the slider handle)
  motor3.innerHTML = slider3.value;
  console.log(slider3.value);
  websocket.send("3"+"s"+slider3.value.toString());
}

//4th motor
var slider4 = document.getElementById("slider4");
var motor4 = document.getElementById("motor4");
motor4.innerHTML = slider4.value; // Display the default slider value
slider4.oninput = function() {// Update the current slider value (each time you drag the slider handle)
  motor4.innerHTML = slider4.value;
  console.log(slider4.value);
  websocket.send("4"+"s"+slider4.value.toString());
}

//master slider -> controls all sliders at the same time
var sliderOffset = 0;

var slider5 = document.getElementById("slider5");
var master = document.getElementById("master");
master.innerHTML = slider5.value; // Display the default slider value
sliderStart = master.innerHTML;
slider5.oninput = function() {// Update the current slider value (each time you drag the slider handle)
    masterOffset = master.innerHTML - sliderStart;
    sliderTemp1 = motor1.innerHTML;
    master.innerHTML = slider5.value;
    motor1.innerHTML = master.innerHTML;
    motor2.innerHTML = master.innerHTML;
    motor3.innerHTML = master.innerHTML;
    motor4.innerHTML = master.innerHTML;
    
    slider1.value = motor1.innerHTML;
    slider2.value = motor2.innerHTML;
    slider3.value = motor3.innerHTML;
    slider4.value = motor4.innerHTML;
    
    console.log(slider5.value);
    websocket.send("5"+"s"+slider5.value.toString());
}


//update motor power and send value
motor1real = Math.floor(motor1.innerHTML * 2.55);
motor2real = Math.floor(motor2.innerHTML * 2.55);
motor3real = Math.floor(motor3.innerHTML * 2.55);
motor4real = Math.floor(motor4.innerHTML * 2.55);
