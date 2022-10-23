var addon = require('bindings')('binding');

function test() {
  let obj = {};
  addon.test(obj);
}

for (let i = 0; i < 100; i++) {
  test();
  addon.gc();
}
