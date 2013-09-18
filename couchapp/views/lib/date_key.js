module.exports = function (date) {
    if (typeof date === 'string') date = new Date(date);
	return [
		date.getUTCFullYear(),      // 0
		date.getUTCMonth() + 1,     // 1
		date.getUTCDate(),          // 2
		date.getUTCHours(),         // 3
		date.getUTCMinutes(),       // 4
		date.getUTCSeconds() + (date.getUTCMilliseconds() / 1000)
    ];
};
