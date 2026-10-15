const fs = require('fs');
const path = require('path');
const vm = require('vm');
const assert = require('assert/strict');
const ts = require(process.env.SERIESLEDGER_TYPESCRIPT || 'typescript');
const filename = path.join(__dirname, '../entry/src/main/ets/abilitystage/MyAbilityStage.ets');
const source = fs.readFileSync(filename, 'utf8');
const js = ts.transpileModule(source, { compilerOptions: {
  module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020
} }).outputText;
let attached = 0;
const qpa = { attachAbilityStage: () => { attached++; } };
const sandbox = { exports: {}, require: name => {
  if (name === '@kit.AbilityKit') return { AbilityStage: class {} };
  if (name === 'libplugins_platforms_qopenharmony.so') return { default: qpa };
  if (name === '@ohos.hilog') return { default: { info: () => {} } };
  throw Error('Unexpected dependency: ' + name);
} };
vm.runInNewContext(js, sandbox, { filename, timeout: 1000 });
const stage = new sandbox.exports.default();
stage.onCreate();
assert.equal(attached, 1);
let passed = 1;
for (const want of [{}, { parameters: {} }, { parameters: { windowMode: 1 } },
  { parameters: { instanceKey: '' } }, { parameters: { instanceKey: 7 } }]) {
  assert.equal(stage.onAcceptWant(want), 'influxdbstudio_main_window'); passed++;
}
for (let i = 0; i < 30; i++) {
  assert.equal(stage.onAcceptWant({}), 'influxdbstudio_main_window');
}
passed++;
for (const key of ['qt_dialog_1', 'qt_dialog_2']) {
  assert.equal(stage.onAcceptWant({ parameters: { instanceKey: key } }), key); passed++;
}
assert.equal(stage.onAcceptWant({}), 'influxdbstudio_main_window'); passed++;
console.log(JSON.stringify({ passed, repeatedPrimaryWants: 30, nativeGetterProvided: false,
  scope: 'Production AbilityStage host routing controls; not native layout acceptance' }));
