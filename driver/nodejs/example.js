var timeswipe = require('timeswipe');

timeswipe.SetBridge(0);
timeswipe.SetSensorOffsets(0,0,0,0);
timeswipe.SetSensorGains(1,1,1,1);
timeswipe.SetSensorTransmissions(1,1,1,1);

timeswipe.onButton(async function (pressed, counter) {
    console.error("onButton pressed: ", pressed, " counter: ", counter);
});
timeswipe.onError(async function (errors) {
    console.error("onError errors: ", errors);
});

timeswipe.Start(async function (data, error) {
    if (error) {
        console.error("errors: ",error);
    } else {
        await data.forEach(function(entry) {
            console.log(entry.join('\t'));
        });
        if (data.length == 0 && errors==0) {
            await sleep(100);
        }
    }
});
