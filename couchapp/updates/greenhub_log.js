function (doc, req) {
    if (doc) return [null, {code:403,body:"Can't update existing logs."}];
    
    var parts = req.body.split(/,?\s+/),
        recvd = new Date(parts[0]);
    
    // "successfully" ignore lines that don't look like logs, to keep linepost running
    if (isNaN(recvd.getTime())) return [null, {code:200,body:"Ignoring invalid message"}];
    
    var doc = {_id:'gnhb-'+recvd.getTime()};
    doc.timestamp = parts[0];
    parts.slice(1).forEach(function (part) {
        var kv = part.split('=');
        if (kv.length !== 2) return;
        var k = kv[0],
            vN = +kv[1],
            v = isNaN(vN) ? kv[1] : vN;
        doc[k] = v;
    });
    return [doc, "om nom nom nom"];
}