function (doc) {
    var d = require("views/lib/date_key");
    if (doc.data && doc.data.nc !== 0) return;              // HACK: toss corrupted data
    if ('com.stemstorage.greenhub_log' in doc) Object.keys(doc.data).forEach(function (key) {
        var value = doc.data[key];
        if (key === 'humidty') key = 'humidity';            // HACK: fix up greenhub.cpp typo (64a8ea1)
        emit([key].concat(d(doc.timestamp)), value);
    });
}