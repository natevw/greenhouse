function (doc) {
    var d = require("views/lib/date_key");
    if ('com.stemstorage.greenhub_log' in doc) emit(d(doc.timestamp), doc.humidty);
}