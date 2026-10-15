import { AbilityStage } from '@kit.AbilityKit';
import Want from '@ohos.app.ability.Want';
import qpa from 'libplugins_platforms_qopenharmony.so';
import hilog from '@ohos.hilog';

export default class MyAbilityStage extends AbilityStage {
  tag = 'InfluxDBStudioQt';
  domain = 0x4944;

  onCreate(): void {
    qpa.attachAbilityStage(this);
  }

  onAcceptWant(want: Want): string {
    // A launcher/restore Want must reuse the primary window. Qt-created child
    // windows supply their own key; the bundled QPA has no JS key getter.
    const instanceKey = want.parameters?.instanceKey;
    if (typeof instanceKey === 'string' && instanceKey.length > 0) {
      return instanceKey;
    }
    hilog.info(this.domain, this.tag, 'Reuse primary window');
    return 'influxdbstudio_main_window';
  }
}
