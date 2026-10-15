import { hilog } from '@kit.PerformanceAnalysisKit';
import { UIExtensionContentSession, Want } from '@kit.AbilityKit';
import EmbeddedUIExtensionAbility from '@ohos.app.ability.EmbeddedUIExtensionAbility';
import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import qpa from 'libplugins_platforms_qopenharmony.so';

export default class EntryEmbeddedAbility extends EmbeddedUIExtensionAbility {
  tag = 'InfluxDBStudioQt';
  domain = 0x4944;
  private name: string = 'influxdbstudio_main_window';
  private launchParams = '';
  private launchApplication = 'libentry.so';
  private launchWant: Want = {};
  private loadContentUrl: string = 'pages/Index';

  onCreate(launchParam: AbilityConstant.LaunchParam) {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onCreate');
  }

  onForeground() {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onForeground');
  }

  onBackground() {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onBackground');
  }

  onDestroy() {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onDestroy');
  }

  onSessionCreate(want: Want, session: UIExtensionContentSession) {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onSessionCreate');
    this.launchWant = want;
    let localStore = new LocalStorage();
    localStore.setOrCreate('session', session);
    localStore.setOrCreate('idName', this.name);
    try {
      session.loadContent(this.loadContentUrl, localStore);
    } catch (error) {
      hilog.error(this.domain, this.tag, 'loadContent failed: %{public}s', JSON.stringify(error));
      return;
    }
    let newAbility = this.launchWant.parameters?.newAbility as boolean;
    if (newAbility !== undefined) {
      return;
    }
    this.startQtApplication(session);
  }

  newLocalStorage() {
    return new LocalStorage();
  }

  private async startQtApplication(session: UIExtensionContentSession) {
    await this.launchParamSet();
    qpa.startQtApplication(this, session);
  }

  async launchParamSet() {
    let params: string[] = [];
    if (this.launchWant.uri !== undefined && this.launchWant.uri !== '') {
      params.push(this.launchWant.uri);
    }
    this.launchParams = params.join(' ');
  }

  onSessionDestroy(session: UIExtensionContentSession) {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onSessionDestroy');
  }
}
