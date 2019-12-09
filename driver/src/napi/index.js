var addon = require('bindings')('timeswipe');

var obj = new addon.TimeSwipeNAPI();
//console.log( obj.SetBridge(0) );
obj.SetBridge(0);
obj.SetSensorOffsets(0,0,0,0);
obj.SetSensorGains(0,0,0,0);
obj.SetSensorTransmissions(0,0,0,0);
obj.Start(function (data, errors) {
    if (errors) {
        console.log("Errors: ",errors);
    } else {
        console.log(data.join('\t'));
    }
});
