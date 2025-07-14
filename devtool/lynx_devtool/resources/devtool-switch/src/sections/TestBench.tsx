import { useAtom, useAtomValue } from 'jotai';

import { testBench, platform } from '../atoms';
import { Switch } from '../components/Switch';
import { i18n } from '../i18n';

export function TestBench() {
  const [enable, setEnable] = useAtom(testBench);

  const currentPlatform = useAtomValue(platform);

  if (currentPlatform !== 'Android') {
    return null;
  }

  return (
    <Switch
      title={i18n.t('TestBench')}
      description={i18n.t('TestBench desc')}
      on={enable}
      onChange={() => setEnable((prev) => !prev)}
    />
  );
}
