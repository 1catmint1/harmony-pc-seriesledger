import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import UIAbility from '@ohos.app.ability.UIAbility';
import Window from '@ohos.window';
import Want from '@ohos.app.ability.Want';
import qpa from 'libplugins_platforms_qopenharmony.so';
import hilog from '@ohos.hilog';

export default class EntryAbility extends UIAbility {
  tag = 'InfluxDBStudioQt';
  domain = 0x4944;
  private launchApplication = 'libentry.so';
  private launchParams = '';
  private name: string = 'influxdbstudio_main_window';
  private loadContentUrl: string = 'pages/Index';
  private isForeground: boolean = false;

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {
    let name = want.parameters?.windowName as string;
    if (name !== undefined && name !== '') {
      this.name = name;
    }

    let applicationContext = this.context.getApplicationContext();
    applicationContext.on('applicationStateChange', {
      onApplicationForeground() {
        console.log('applicationStateChangeCallback onApplicationForeground');
      },
      onApplicationBackground() {
        console.log('applicationStateChangeCallback onApplicationBackground');
      }
    });
  }

  newLocalStorage() {
    return new LocalStorage();
  }

  onDestroy() {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onDestroy');
  }

  onWindowStageCreate(windowStage: Window.WindowStage) {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onWindowStageCreate');
    let localStore = new LocalStorage();
    localStore.setOrCreate('idName', this.name);
    let newAbility = this.launchWant.parameters?.newAbility as boolean;

    windowStage.loadContent(this.loadContentUrl, localStore).then(() => {
      qpa.handleJsTopWindowCreated(this.name, this);
      if (newAbility !== undefined) {
        this.context.setMissionLabel(this.launchWant.parameters?.title as string);
      }
    }).catch((error: Error) => {
      hilog.error(this.domain, this.tag, 'loadContent failed: %{public}s', JSON.stringify(error));
    });

    if (newAbility !== undefined) {
      return;
    }
    this.launchParamSet();
    qpa.startQtApplication(this);
  }

  async launchParamSet() {
    let params: string[] = [];
    if (this.launchWant.uri !== undefined && this.launchWant.uri !== '') {
      params.push(this.launchWant.uri);
    }
    this.launchParams = params.join(' ');
  }

  onWindowStageDestroy() {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onWindowStageDestroy');
  }

  onForeground() {
    this.isForeground = true;
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onForeground');
  }

  onBackground() {
    this.isForeground = false;
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onBackground');
  }

  onNewWant(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    qpa.handleOnNewWant(want, launchParam);
  }
}
