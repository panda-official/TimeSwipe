var addon = require('bindings')('timeswipe');

var obj = new addon.TimeSwipeNAPI();
var set_req='{ "Gain" : "?", "Bridge" : "?"}\n'
var settings = obj.GetSettings(set_req);
console.error("initial settings: ",settings);
obj.SetBridge(0);
obj.SetSensorOffsets(0,0,0,0);
obj.SetSensorGains(1,1,1,1);
obj.SetSensorTransmissions(1,1,1,1);
obj.onButton(async function (pressed, counter) {
    console.error("onButton pressed: ", pressed, " counter: ", counter);
});
obj.onError(async function (errors) {
    console.error("onError errors: ", errors);
});
settings = obj.GetSettings(set_req);
console.error("base settings: ",settings);
obj.Start(async function (data, errors) {
    if (errors) {
        console.error("errors: ",errors);
    } else {
        await data.forEach(function(entry) {
            console.log(entry.join('\t'));
        });
        if (data.length == 0 && errors==0) {
            await sleep(100);
        }
    }
});
