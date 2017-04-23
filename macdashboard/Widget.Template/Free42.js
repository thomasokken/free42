/* Free42.js:  Copyright 2005 D. Jeff Dionne
 *
 * This file is in the Public Domain.
 */

var hackid = 0;
var t_instance = null;
var x_reg;

function debugMsg(msg)
{
	document.getElementById("Debug").innerText = msg;
}

function outputHandler(cmd)
{
	var operand = cmd.slice(1);

	switch (cmd.charAt(0)) {

	case 'd':
		document.getElementById("Display").src = "display.gif?" + hackid;
		hackid = hackid+1;
		break;
	case 'A':
		document.getElementById(operand).src = "Images/" + operand + ".png";
		break;
	case 'a':
		document.getElementById(operand).src = "Images/BlankAnn.png";
		break;
	case 'x':
		x_reg = operand;
	}
	t_instance.write("Ok\n");
}

function doCopy(event)
{
	event.clipboardData.setData('text/plain', x_reg);
	event.preventDefault();
	event.stopPropagation();
}

function doPaste(event)
{
	var clip = event.clipboardData.getData('text/plain');

        // Temporarily change CR to US, and LF to RS, just so the entire
        // clipboard contents can be treated as a single line of text;
        // we'll undo this encoding in the native code.
        clip = clip.replace(/\r/g, '\037');
        clip = clip.replace(/\n/g, '\036');
	t_instance.write("P" + clip + "\n");

	event.preventDefault();
	event.stopPropagation();
}

function ex()
{
// the exit handler for the free42 binary.  Empty
}

function startUp()
{
	if (t_instance == null && window.widget) {
		t_instance = widget.system("bin/free42", ex);
		t_instance.onreadoutput = outputHandler;
	}

	if (widget.preferenceForKey("Free42-invSingular")) document.getElementById("invSingular").checked = true;
	if (widget.preferenceForKey("Free42-matrixOverflow")) document.getElementById("matrixOverflow").checked = true;

	document.addEventListener("keydown", keyPressed, true);
}

function prefsChange(id)
{
	var checked = document.getElementById(id).checked;

	if (window.widget) {
		if (checked) {
			widget.setPreferenceForKey(true, "Free42-" + id);
			t_instance.write("E"+id+"\n");
		} else {
			widget.setPreferenceForKey(null, "Free42-" + id);
			t_instance.write("e"+id+"\n");
		}
	}
}

function mouseClick(event, id)
{
	event.target.src = "Images/d/"+id+".gif";

	event.stopPropagation();
	event.preventDefault();
	t_instance.write("C"+id+"\n");
}

function mouseExit(event, id)
{
	event.target.src = "Images/u/"+id+".gif";

	event.stopPropagation();
	event.preventDefault();
	t_instance.write("U"+id+"\n");
}

function mouseRelease(event, id)
{
	event.target.src = "Images/u/"+id+".gif";

	event.stopPropagation();
	event.preventDefault();
	t_instance.write("U"+id+"\n");
}

function keyPressed(event)
{
	var code = event.charCode;
	// Apple calc says drop the keystroke if it's a meta key
	if (event.metaKey) return;
	if (event.ctrlKey) 
	   code = code + 65536;
	if (event.altKey) 
	   code = code + 131072;
	if (event.shiftKey) 
	   code = code + 262144;
	t_instance.write("K"+code+"\n");
}

// The standard Apple code for Info button and flip
var flipShown = false;
var animation = {duration:0, starttime:0, to:1.0, now:0.0, from:0.0, firstElement:null, timer:null};

function showPrefs()
{
	var front = document.getElementById("front");
	var back = document.getElementById("back");
       
	if (window.widget) widget.prepareForTransition("ToBack");
                
	front.style.display="none";
	back.style.display="block";
        
	if (window.widget) setTimeout ('widget.performTransition();', 0);  
}

function hidePrefsOut(event)
{
	event.target.src = "Images/done.png";

	event.stopPropagation();
	event.preventDefault();
}

function hidePrefsDown(event)
{
	event.target.src = "Images/done_pressed.png";

	event.stopPropagation();
	event.preventDefault();
}

function hidePrefs(event)
{
	var front = document.getElementById("front");
	var back = document.getElementById("back");

	event.target.src = "Images/done.png";

	if (window.widget) widget.prepareForTransition("ToFront");

	back.style.display="none";
	front.style.display="block";
        
	if (window.widget) setTimeout ('widget.performTransition();', 0);
}

function mousemove (event)
{
	if (!flipShown) {
		if (animation.timer != null) {
			clearInterval (animation.timer);
			animation.timer  = null;
		}
    
		var starttime = (new Date).getTime() - 13;
 
		animation.duration = 500;
		animation.starttime = starttime;
		animation.firstElement = document.getElementById ('flip');
		animation.timer = setInterval ("animate();", 13);
		animation.from = animation.now;
		animation.to = 1.0;
		animate();
		flipShown = true;
	}
}

function mouseexit (event)
{
	if (flipShown) {
		// fade in the info button
		if (animation.timer != null) {
			clearInterval (animation.timer);
			animation.timer  = null;
		}
         
		var starttime = (new Date).getTime() - 13;
       
		animation.duration = 500;
		animation.starttime = starttime;
		animation.firstElement = document.getElementById ('flip');
		animation.timer = setInterval ("animate();", 13);
		animation.from = animation.now;
		animation.to = 0.0;
		animate();
		flipShown = false;
	}
}

function animate()
{
	var T;
	var ease;
	var time = (new Date).getTime();

	T = limit_3(time-animation.starttime, 0, animation.duration);

	if (T >= animation.duration) {
		clearInterval (animation.timer);
		animation.timer = null;
		animation.now = animation.to;
	} else {
		ease = 0.5 - (0.5 * Math.cos(Math.PI * T / animation.duration));
		animation.now = computeNextFloat (animation.from, animation.to, ease);
	}

	animation.firstElement.style.opacity = animation.now;
}

function limit_3 (a, b, c)
{
	return a < b ? b : (a > c ? c : a);
}

function computeNextFloat (from, to, ease)
{
	return from + (to - from) * ease;
}
