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

var lines = 0
timeswipe.Start(async function (data, error) {
    if (error) {
        console.error("errors: ",error);
    } else {
        data.forEach(function(entry) {
            //console.log(entry.join('\t'));
            lines = lines + 1;
        });
    }
});

const promise = new Promise((resolve, reject) => {
  setTimeout(() => {
    resolve()
  }, 10000)
})

function onComplete () {
  timeswipe.Stop()
  console.log('got lines: ', lines)
  process.exit(0)
}

promise.then(onComplete)

