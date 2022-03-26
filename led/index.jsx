var margin_y = 100, // 400
	margin_x = 400;

function $(item) {
	return document.querySelector(item);
}
function $$(item) {
	return document.querySelectorAll(item);
}
var led = $$(".led,.dot,.led2");
for (let i = 0; i < led.length; i++) {
	(function (i) {
		var item = led[i];
		item.onclick = function () {
			item.classList.toggle("light");
			getResult();
		};
	}(i));
}
$("#method").onchange = getResult;
function getLight() {
	var value = $("#method").value,
		anode = value & 1,
		toLow = value & 2,
		list = [];
	for (let i = 0; i < led.length; i++) {
		list[i] = led[i].classList.contains("light") - 0;
		if (!anode) list[i] = !list[i] - 0;
	}
	if (!toLow) list.reverse();
	return list.join('');
}
function getResult() {
	var b = getLight();
	$("#binary").innerText = "0b" + b;
	var hex = parseInt(b, 2).toString(16);
	if (hex.length < 2) hex = '0' + hex;
	$("#hexadecimal").innerText = "0x" + hex;
}
onload = onresize = function () {
	var transform = function (value) {
		$(".container").style.transform = `scale(${value})`;
	},
		width = $("#led").clientWidth + $("#message").clientWidth + margin_x,
		height = $("#led").clientHeight + margin_y;
	if (height * innerWidth / width < innerHeight) transform(innerWidth / width);
	else transform(innerHeight / height);
};
$("#info").onclick = function () {
	$("#led").classList.toggle("skew");
};
function copy(str = "") {
	var el = document.createElement("textarea");
	el.style.opacity = 0;
	document.body.append(el);
	el.innerText = str;
	el.select();
	document.execCommand("copy");
	el.remove();
}
var numeral = $$("#binary,#hexadecimal");
for (let i = 0; i < numeral.length; i++) {
	(function (i) {
		var item = numeral[i];
		item.title = "点击复制内容";
		item.onclick = function () {
			copy(item.innerText);
		};
	}(i));
}
