"""Guard native CubeIDE target and launch metadata against generic imports."""
import json
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
PREFIX = 'com.st.stm32cube.ide.mcu.gnu.managedbuild.'

class CubeIDEProjectTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tree = ET.parse(ROOT / '.cproject')
        cls.configs = {node.find('.//configuration').get('name'): node
                       for node in cls.tree.findall('.//cconfiguration')}

    def test_native_project_natures(self):
        project = ET.parse(ROOT / '.project')
        self.assertEqual(project.findtext('name'), 'ChromeDinoSTM32')
        natures = {node.text for node in project.findall('.//nature')}
        self.assertIn('com.st.stm32cube.ide.mcu.MCUProjectNature', natures)
        self.assertIn('org.eclipse.cdt.core.cnature', natures)
        self.assertNotIn('org.eclipse.cdt.core.ccnature', natures)

    def test_cdt_file_version_is_preserved(self):
        # CDT needs this processing instruction to read managed configurations.
        self.assertIn('<?fileVersion 4.0.0?>', (ROOT / '.cproject').read_text())

    def test_both_builds_target_the_c6(self):
        self.assertEqual(set(self.configs), {'Debug', 'Release'})
        for name, config in self.configs.items():
            with self.subTest(configuration=name):
                options = {node.get('superClass'): node for node in config.findall('.//option')}
                self.assertEqual(options[PREFIX+'option.target_mcu'].get('value'), 'STM32F051C6Tx')
                self.assertIn('STM32F051C6Tx', options[PREFIX+'option.defaults'].get('value'))
                build = config.find('.//configuration')
                self.assertEqual(build.get('parent'), PREFIX+'config.exe.'+name.lower())
                self.assertEqual(build.get('artifactExtension'), 'elf')
                self.assertEqual(config.find('.//builder').get('managedBuildOn'), 'true')
                self.assertTrue(config.find('.//builder').get('buildPath').endswith('/'+name))
                self.assertTrue(options[PREFIX+'tool.c.linker.option.script'].get('value').endswith('/STM32F051C6TX_FLASH.ld}'))
                defines = options[PREFIX+'tool.c.compiler.option.definedsymbols']
                values = {node.get('value') for node in defines}
                # This shared CMSIS symbol includes the C6; it is not a C8 linker choice.
                self.assertTrue({'STM32F051x8', 'USE_HAL_DRIVER'} <= values)
                self.assertEqual(options[PREFIX+'tool.c.compiler.option.cyclomaticcomplexity'].get('value'), 'false')
                for output in ['convertbinary', 'converthex']:
                    self.assertEqual(options[PREFIX+'option.'+output].get('value'), 'true')

    def test_only_firmware_roots_are_compiled(self):
        for config in self.configs.values():
            roots = {node.get('name') for node in config.findall('.//sourceEntries/entry')}
            self.assertEqual(roots, {'Core', 'Drivers'})
        self.assertTrue((ROOT/'Core/Startup/startup_stm32f051c6tx.s').is_file())
        linker = (ROOT/'STM32F051C6TX_FLASH.ld').read_text()
        self.assertIn('LENGTH = 32K', linker)
        self.assertIn('LENGTH = 8K', linker)

    def test_launches_build_and_load_the_matching_configuration(self):
        for launch_name, config_name in [('Debug','Debug'), ('Default','Debug'), ('Release','Release')]:
            launch = ET.parse(ROOT/f'ChromeDinoSTM32 {launch_name}.launch')
            values = {node.get('key'): node.get('value') for node in launch.getroot()}
            self.assertEqual(values['org.eclipse.cdt.launch.PROJECT_ATTR'], 'ChromeDinoSTM32')
            self.assertEqual(values['org.eclipse.cdt.launch.PROGRAM_NAME'], f'{config_name}/ChromeDinoSTM32.elf')
            self.assertEqual(values['org.eclipse.cdt.launch.PROJECT_BUILD_CONFIG_ID_ATTR'], self.configs[config_name].get('id'))
            self.assertEqual(values['org.eclipse.cdt.launch.PROJECT_BUILD_CONFIG_AUTO_ATTR'], 'false')
            self.assertEqual(values['com.st.stm32cube.ide.mcu.debug.launch.swd_mode'], 'true')
            load = json.loads(values['com.st.stm32cube.ide.mcu.debug.launch.loadList'])['fItems'][0]
            self.assertEqual(load['fPath'], f'{config_name}/ChromeDinoSTM32.elf')
            self.assertTrue(load['fPerformBuild'])

    def test_clock_reference_uses_the_c6_package(self):
        reference = (ROOT/'ChromeDinoSTM32.ioc').read_text()
        for expected in ['Mcu.Name=STM32F051C6Tx', 'Mcu.Package=LQFP48',
                         'ProjectManager.DeviceId=STM32F051C6Tx', 'RCC.HCLKFreq_Value=48000000']:
            self.assertIn(expected, reference)

    def test_language_settings_match_real_configurations(self):
        language = ET.parse(ROOT/'.settings/language.settings.xml')
        ids = {node.get('id') for node in language.findall('configuration')}
        self.assertEqual(ids, {node.get('id') for node in self.configs.values()})

if __name__ == '__main__':
    unittest.main(verbosity=2)
