import { CurrentSystemMetaData } from 'stores/configuration/emulationstation.d';

type SystemsMetaData = Record<string, CurrentSystemMetaData>

export const systemsMetaData: SystemsMetaData = {
  '3do': {
    system: {
      manufacturer: 'Panasonic, Sanyo, GoldStar',
      yearOfRelease: '1993',
      os: 'Multitasking 32-bit OS',
      cpu: '32-bit RISC (ARM60) running @ 12.5 MHz',
      ram: '2 MB / VRAM',
      rom: '1 MB',
      gpu: '2 Accelerated Video Co-Processors @25MHz',
      soundChip: '16-bit Stereo Sound with 44.1 KHz Sound Sampling Rate',
      resolution: '640x480 (16.7 million colours)',
    },
    colors: {
      eu: {
        band1: '#FF0000',
        band2: '#0000A0',
        band3: '#FFE41B',
        band4: '#23B14D',
      },
    },
  },
  '3ds': {
    system: {
      manufacturer: 'Nintendo',
      yearOfRelease: '2011',
      unitsSold: '72.53 million',
      bestSellingGame: 'Mario Kart 7',
      cpu: '268MHz Dual-Core ARM11 MPCore + single-core ARM9',
      fcram: '128MB / Video RAM',
      screen: 'Reflective TFT Colour LCD',
      resolution: '800 x 240 pixels (16.7 million colours)',
      sound: 'Stereo speakers w/virtual surround, headphones',
    },
    colors: {
      eu: {
        band1: '#929497',
        band2: '#C02424',
        band3: '#221D1F',
        band4: '#1F8ABE',
      },
    },
  },
  '64dd': {
    system: {
      manufacturer: 'Nintendo',
      yearOfRelease: '1999',
      unitsSold: '15.000',
      cpu: '64-bit NEC VR4300 @ 93.75 MHz',
      cpu2: '64-bit MIPS RISC "Reality Immersion" RCP @ 62.5 MHz',
      '64ddCpu': '32-bit co-processor for reading/transferring disk data',
      ram: '4 MB RAMBUS RDRAM (expandable up to 8 MB)',
      sound: 'Stereo 16-Bit and 48 kHz',
    },
    colors: {
      eu: {
        band1: '#DCAF36',
        band2: '#268340',
        band3: '#20317F',
        band4: '#B73E3A',
      },
    },
  },
  '240ptestsuite': {
    system: {
      author: 'Artemio Urbina',
      yearOfRelease: '2011',
    },
    colors: {
      eu: {
        band1: '#B00000',
        band2: '#00B000',
        band3: '#0000B0',
        band4: '#FFFFFF',
      },
    },
  },
  allgames: {
    system: {},
    colors: {
      eu: {
        band1: '#C1B544',
        band2: '#6EAE97',
        band3: '#364792',
        band4: '#851740',
      },
    },
  },
  amiga600: {
    system: {
      manufacturer: 'Commodore',
      yearOfRelease: '1992',
      os: 'AmigaOS 2.05',
      cpu: 'Motorola 68000 running @ 7.16 MHz',
      ram: '1 MB (expandable up to 6 MB officially) / VRAM 1024 KB',
      rom: '512 KB Kickstart 2.05',
      gpu: "Commodore's Enhanced Chip Set",
      soundChip: '4 DMA-driven 8-bit channels',
      resolution: '320x200 to 1280x512 (4096 colours at lower display resolutions)',
    },
    colors: {
      eu: {
        band1: '#192753',
        band2: '#5BA079',
        band3: '#F9EA38',
        band4: '#ED2224',
      },
    },
  },
  amiga1200: {
    system: {
      manufacturer: 'Commodore',
      yearOfRelease: '1992',
      os: 'AmigaOS 3.0/3.1',
      cpu: 'Motorola 68EC020 running @ 14.32 MHz',
      ram: '2 MB',
      rom: '512 KB Kickstart 3.0-3.1 ROM',
      gpu: 'Advanced Graphics Architecture',
      soundChip: '4x 8-bit PCM channels',
      resolution: '320x200 to 1504x484 (16.8 million colours)',
    },
    colors: {
      eu: {
        band1: '#192753',
        band2: '#5F87C0',
        band3: '#F3AD2D',
        band4: '#ED2224',
      },
    },
  },
  amigacd32: {
    system: {
      manufacturer: 'Commodore International',
      yearOfRelease: '1993',
      os: 'AmigaOS 3.1',
      cpu: 'Motorola 68EC020 @ 14.32 MHz (NTSC), 14.18 MHz (PAL)',
      ram: '2 MB',
      rom: '1 MB Kickstart w/ CD32 firmware, 1 KB EEPROM for game saves',
      gpu: 'Advanced Graphics Architecture',
      soundChip: '4 x 8-bit PCM channels',
      resolution: '320x200 to 1280x400i (NTSC), 320x256 to 1280x512i (PAL)',
    },
    colors: {
      eu: {
        band1: '#FF5853',
        band2: '#47DEBA',
        band3: '#FFDF46',
        band4: '#4A9BE4',
      },
    },
  },
  amigacdtv: {
    system: {
      manufacturer: 'Commodore International',
      yearOfRelease: '1991',
      os: 'CDTV / AmigaOS 1.3',
      cpu: 'Motorola 68000 @ 7.16 MHz (NTSC) or 7.09 (PAL)',
      ram: '1 MB (upgradable)',
      rom: '256 KB Kickstart 2.5 and 256 KB CDTV firmware',
      gpu: "Commodore's Enhanced Chip Set",
      soundChip: 'Four 8-bit PCM channels (two stereo channels)',
      resolution: '320 x 200 (32 colors) / 640 x 240 (16 colors)',
    },
    colors: {
      eu: {
        band1: '#C86388',
        band2: '#6F62E8',
        band3: '#1CA6F1',
        band4: '#F62717',
      },
    },
  },
  amstradcpc: {
    system: {
      manufacturer: 'Amstrad',
      yearOfRelease: '1984',
      os: 'AMSDOS with Locomotive BASIC',
      cpu: 'Zilog Z80A @ 4 MHz',
      ram: '64 KB',
      soundChip: 'General Instrument AY-3-8910',
    },
    colors: {
      eu: {
        band1: '#E91C23',
        band2: '#00A651',
        band3: '#1C75BC',
        band4: '#942647',
      },
    },
  },
  apple2: {
    system: {
      manufacturer: 'Apple Computer, Inc.',
      yearOfRelease: '1977',
      os: 'Integer BASIC',
      cpu: 'MOS Technology 6502 @ 1 Mhz',
      ram: '4KB to 64KB',
      rom: '8KB (Basic Integer)',
      sound: '1-bit speaker (built-in)',
      display: 'Lo-res (40x48, 16-color) Hi-res (280x192, 6 color)',
    },
    colors: {
      eu: {
        band1: '#009ddc',
        band2: '#e03a3e',
        band3: '#fdb827',
        band4: '#61bb46',
      },
    },
  },
  apple2gs: {
    system: {
      manufacturer: 'Apple Computer, Inc.',
      yearOfRelease: '1986',
      os: 'Apple ProDOS, Apple GS/OS, GNO/ME',
      cpu: 'Western Design Center 65C816 16-bit @ 2.8 MHz',
      ram: '256 kB or 1 MB (expandable up to 8 MB)',
      rom: '128 KB expandable to 1 MB',
      soundChip: 'Ensoniq ES5503 DOC 8-bit wavetable synthesis sound chip, 32-channels, stereo',
      display: 'VGC 12-bpp palette (4096 colours), 320x200, 640x200',
    },
    colors: {
      eu: {
        band1: '#D27C35',
        band2: '#C44243',
        band3: '#E3AD3A',
        band4: '#8C3E8C',
      },
    },
  },
  'arcade-manufacturer-acclaim': {
    system: {},
    colors: {
      eu: {
        band1: '#0739BE',
        band2: '#17DE13',
        band3: '#F1F153',
        band4: '#C10003',
      },
    },
  },
  'arcade-manufacturer-atari': {
    system: {},
    colors: {
      eu: {
        band1: '#7F0B03',
        band2: '#000000',
        band3: '#A7A7A7',
        band4: '#FEFEFE',
      },
    },
  },
  'arcade-manufacturer-atlus': {
    system: {},
    colors: {
      eu: {
        band1: '#10539A',
        band2: '#E70012',
        band3: '#FDFFFF',
        band4: '#BABABA',
      },
    },
  },
  'arcade-manufacturer-banpresto': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#FFFFFF',
        band3: '#F80205',
        band4: '#BC0004',
      },
    },
  },
  'arcade-manufacturer-capcom': {
    system: {},
    colors: {
      eu: {
        band1: '#0C4DA2',
        band2: '#FFFFFF',
        band3: '#FFCB08',
        band4: '#000000',
      },
    },
  },
  'arcade-manufacturer-capcom-cps1': {
    system: {},
    colors: {
      eu: {
        band1: '#FFCB08',
        band2: '#0C4DA2',
        band3: '#509FEC',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-capcom-cps2': {
    system: {},
    colors: {
      eu: {
        band1: '#0C4DA2',
        band2: '#509FEC',
        band3: '#FFFFFF',
        band4: '#FFCB08',
      },
    },
  },
  'arcade-manufacturer-capcom-cps3': {
    system: {},
    colors: {
      eu: {
        band1: '#FFFFFF',
        band2: '#FFCB08',
        band3: '#509FEC',
        band4: '#0C4DA2',
      },
    },
  },
  'arcade-manufacturer-cave': {
    system: {},
    colors: {
      eu: {
        band1: '#AF0000',
        band2: '#319402',
        band3: '#9FDE01',
        band4: '#B6B6B6',
      },
    },
  },
  'arcade-manufacturer-dataeast': {
    system: {},
    colors: {
      eu: {
        band1: '#F91B20',
        band2: '#000000',
        band3: '#A5A5A5',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-exidy': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#262424',
        band3: '#7D7D7D',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-hng64': {
    system: {},
    colors: {
      eu: {
        band1: '#BF9328',
        band2: '#494949',
        band3: '#2D2D2D',
        band4: '#E8E2E9',
      },
    },
  },
  'arcade-manufacturer-igs': {
    system: {},
    colors: {
      eu: {
        band1: '#FFFFFF',
        band2: '#7A7A7A',
        band3: '#FE848D',
        band4: '#C2020E',
      },
    },
  },
  'arcade-manufacturer-irem': {
    system: {},
    colors: {
      eu: {
        band1: '#E94B46',
        band2: '#3A459A',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-irem-m72': {
    system: {},
    colors: {
      eu: {
        band1: '#E94B46',
        band2: '#3A459A',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-irem-m92': {
    system: {},
    colors: {
      eu: {
        band1: '#E94B46',
        band2: '#3A459A',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-itech': {
    system: {},
    colors: {
      eu: {
        band1: '#F19C1A',
        band2: '#FFF31E',
        band3: '#1C1102',
        band4: '#FFFCF7',
      },
    },
  },
  'arcade-manufacturer-jaleco': {
    system: {},
    colors: {
      eu: {
        band1: '#010205',
        band2: '#246BBD',
        band3: '#498DDD',
        band4: '#F7FAFD',
      },
    },
  },
  'arcade-manufacturer-kaneko': {
    system: {},
    colors: {
      eu: {
        band1: '#D81A16',
        band2: '#EB3D39',
        band3: '#8E8C8C',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-konami': {
    system: {},
    colors: {
      eu: {
        band1: '#BC0224',
        band2: '#000000',
        band3: '#FCFEFC',
        band4: '#959595',
      },
    },
  },
  'arcade-manufacturer-konami-gx': {
    system: {},
    colors: {
      eu: {
        band1: '#BC0224',
        band2: '#000000',
        band3: '#FCFEFC',
        band4: '#959595',
      },
    },
  },
  'arcade-manufacturer-midway': {
    system: {},
    colors: {
      eu: {
        band1: '#FFFFFF',
        band2: '#9C929C',
        band3: '#000000',
        band4: '#E20207',
      },
    },
  },
  'arcade-manufacturer-mitchell': {
    system: {},
    colors: {
      eu: {
        band1: '#3C409A',
        band2: '#3DC0F0',
        band3: '#000000',
        band4: '#9FA5A3',
      },
    },
  },
  'arcade-manufacturer-namco': {
    system: {},
    colors: {
      eu: {
        band1: '#E50308',
        band2: '#FC363A',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-namco-na': {
    system: {},
    colors: {
      eu: {
        band1: '#E50308',
        band2: '#FC363A',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-namco-nb': {
    system: {},
    colors: {
      eu: {
        band1: '#E50308',
        band2: '#FC363A',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-namco-system1': {
    system: {},
    colors: {
      eu: {
        band1: '#E50308',
        band2: '#FC363A',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-namco-system2': {
    system: {},
    colors: {
      eu: {
        band1: '#E50308',
        band2: '#FC363A',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-namco-system10': {
    system: {},
    colors: {
      eu: {
        band1: '#E50308',
        band2: '#FC363A',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-namco-system11': {
    system: {},
    colors: {
      eu: {
        band1: '#E50308',
        band2: '#FC363A',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-namco-system12': {
    system: {},
    colors: {
      eu: {
        band1: '#E50308',
        band2: '#FC363A',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-namco-system18': {
    system: {},
    colors: {
      eu: {
        band1: '#0060A8',
        band2: '#008AE4',
        band3: '#FF0000',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-neogeo': {
    system: {},
    colors: {
      eu: {
        band1: '#BF9328',
        band2: '#494949',
        band3: '#2D2D2D',
        band4: '#E8E2E9',
      },
    },
  },
  'arcade-manufacturer-nichibutsu': {
    system: {},
    colors: {
      eu: {
        band1: '#93918F',
        band2: '#000000',
        band3: '#FFFFFF',
        band4: '#FFF100',
      },
    },
  },
  'arcade-manufacturer-nintendo': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#FF0000',
        band3: '#FFFFFF',
        band4: '#807E7C',
      },
    },
  },
  'arcade-manufacturer-nmk': {
    system: {},
    colors: {
      eu: {
        band1: '#FE0001',
        band2: '#06FF06',
        band3: '#000CF7',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-psikyo': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#FF2100',
        band3: '#FFFFFF',
        band4: '#959595',
      },
    },
  },
  'arcade-manufacturer-raizing': {
    system: {},
    colors: {
      eu: {
        band1: '#E94B46',
        band2: '#3A459A',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-sammy': {
    system: {},
    colors: {
      eu: {
        band1: '#207820',
        band2: '#54D401',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-sega': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#FFFFFF',
        band3: '#0D56BD',
        band4: '#80808A',
      },
    },
  },
  'arcade-manufacturer-sega-stv': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#FFFFFF',
        band3: '#0D56BD',
        band4: '#80808A',
      },
    },
  },
  'arcade-manufacturer-sega-system16': {
    system: {},
    colors: {
      eu: {
        band1: '#0060A8',
        band2: '#008AE4',
        band3: '#FF0000',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-sega-system18': {
    system: {},
    colors: {
      eu: {
        band1: '#0060A8',
        band2: '#008AE4',
        band3: '#FF0000',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-sega-system32': {
    system: {},
    colors: {
      eu: {
        band1: '#EB0009',
        band2: '#0060A8',
        band3: '#0084E4',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-seibu': {
    system: {},
    colors: {
      eu: {
        band1: '#193591',
        band2: '#F03190',
        band3: '#FFFFFF',
        band4: '#9C9C9A',
      },
    },
  },
  'arcade-manufacturer-seta': {
    system: {},
    colors: {
      eu: {
        band1: '#0B4199',
        band2: '#1067EE',
        band3: '#FFFFFF',
        band4: '#8A8A8A',
      },
    },
  },
  'arcade-manufacturer-snk': {
    system: {},
    colors: {
      eu: {
        band1: '#0398E1',
        band2: '#000000',
        band3: '#FFFFFF',
        band4: '#8E8E8C',
      },
    },
  },
  'arcade-manufacturer-taito': {
    system: {},
    colors: {
      eu: {
        band1: '#9A9A9A',
        band2: '#000000',
        band3: '#FFFFFF',
        band4: '#127BCA',
      },
    },
  },
  'arcade-manufacturer-taito-f3': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#00A3D7',
        band3: '#E50703',
        band4: '#1FAA33',
      },
    },
  },
  'arcade-manufacturer-taito-gnet': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#00A3D7',
        band3: '#E50703',
        band4: '#1FAA33',
      },
    },
  },
  'arcade-manufacturer-technos': {
    system: {},
    colors: {
      eu: {
        band1: '#94532C',
        band2: '#EBBA87',
        band3: '#0369A6',
        band4: '#000000',
      },
    },
  },
  'arcade-manufacturer-tecmo': {
    system: {},
    colors: {
      eu: {
        band1: '#BB171E',
        band2: '#E3232E',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'arcade-manufacturer-toaplan': {
    system: {},
    colors: {
      eu: {
        band1: '#9C9C9C',
        band2: '#000000',
        band3: '#734200',
        band4: '#E7A500',
      },
    },
  },
  'arcade-manufacturer-visco': {
    system: {},
    colors: {
      eu: {
        band1: '#00499B',
        band2: '#0779FF',
        band3: '#FFFFFF',
        band4: '#8A8A8A',
      },
    },
  },
  arcade: {
    system: {},
    colors: {
      eu: {
        band1: '#FB0B47',
        band2: '#FCE236',
        band3: '#1229B8',
        band4: '#5D9EED',
      },
    },
  },
  arduboy: {
    system: {
      developers: 'Kevin Bates',
      yearOfRelease: '2015',
      cpu: '8-bit ATMega32u4 @ 16 MHz',
      ram: '2.5KB linear RAM',
      display: '1.3" 128x64px 1-bit OLED',
      storage: '32KB of flash',
      sound: 'Stereo Piezo Speakers',
    },
    colors: {
      eu: {
        band1: '#c6a237',
        band2: '#252525',
        band3: '#b94444',
        band4: '#ffffff',
      },
    },
  },
  atari800: {
    system: {
      manufacturer: 'Atari, Inc.',
      yearOfRelease: '1979',
      os: 'Atari OS',
      cpu: 'MOS Technology 6502B @ 1.8 Mhz',
      gpu: 'ANTIC : Alpha Numeric Television Interface Controler, CTIA : Color Television Interface Adaptor',
      ram: '8kB base, 48 kB max.',
      rom: '8KB (Atari Basic)',
      sound: '4x oscillators with noise mixing',
      display: '320x192 monochrome, 160x96 with 128 colors',
    },
    colors: {
      eu: {
        band1: '#b6b896',
        band2: '#ebd335',
        band3: '#5d3615',
        band4: '#bf9438',
      },
    },
  },
  atari2600: {
    system: {
      manufacturer: 'Atari Corporation',
      yearOfRelease: '1977',
      unitsSold: '30 million',
      bestSellingGame: 'Pac-Man',
      cpu: '8-bit MOS 6507 @ 1.19 MHz',
      ram: '128 bytes',
      gpu: 'TIA',
      soundChip: 'TIA',
      resolution: '160 x 192 pixels, 4 colors per line (128 colors palette)',
      sound: '2 channels of 1-bit monaural sound with 4-bit volume control',
      cartSize: '2kB - 32kB',
    },
    colors: {
      eu: {
        band1: '#E6CC33',
        band2: '#B63738',
        band3: '#2F8FBA',
        band4: '#7EAA66',
      },
      jp: {
        band1: '#C1272A',
        band2: '#651966',
        band3: '#1C267D',
        band4: '#0B562F',
      },
    },
  },
  atari5200: {
    system: {
      manufacturer: 'Atari, Inc.',
      yearOfRelease: '1982',
      unitsSold: '1 million',
      cpu: 'MOS Technology 6502C 6502C @ 1.79 MHz',
      ram: '16kB',
      rom: '2KB on-board BIOS; 32 KB window for standard game cartridges',
      soundChip: '4-channel POKEY chip',
      display: '320x192 pixels with 256 colors',
    },
    colors: {
      eu: {
        band1: '#1e6547',
        band2: '#cfa634',
        band3: '#973e3a',
        band4: '#1c6eb8',
      },
    },
  },
  atari7800: {
    system: {
      manufacturer: 'Atari Corporation',
      yearOfRelease: '1986',
      unitsSold: '3,77 million',
      cpu: 'Atari SALLY 6502 ("6502C") @ 1.19-1.79MHz',
      ram: '4KB',
      cartridgeRom: '48KB',
      gpu: 'TIA/MARIA custom graphics controller',
      soundChip: 'TIA/MARIA custom graphics controller',
      ports: '2 joystick ports, 1 cartridge port, 1 expansion connector, power in, RF output',
    },
    colors: {
      eu: {
        band1: '#BABBBD',
        band2: '#AA2C39',
        band3: '#A7B0B5',
        band4: '#333333',
      },
    },
  },
  atarist: {
    system: {
      manufacturer: 'Atari Corporation',
      yearOfRelease: '1985',
      os: 'Atari TOS',
      cpu: 'Motorola 68000 16-/32-bit CPU @ 8 MHz',
      ram: '512KB or 1MB',
      drive: 'Single-sided 3½ floppy disk drive',
      soundChip: 'Yamaha YM2149 3-voice squarewave',
      lowResolutions: '320 x 200 (16 color), palette of 512 colors',
      mediumResolutions: '640 x 200 (4 color), palette of 512 colors',
    },
    colors: {
      eu: {
        band1: '#152D7D',
        band2: '#357BB7',
        band3: '#949494',
        band4: '#ECECEC',
      },
    },
  },
  atomiswave: {
    system: {
      manufacturer: 'Sammy Corporation',
      yearOfRelease: '2003',
      cpu: 'Hitachi SH-4 32-bit RISC CPU (@ 200 MHz 360 MIPS / 1.4 GFLOPS)',
      gpu: 'NEC-Videologic PowerVR (PVR2DC/CLX2) @ 100 MHz',
      soundChip: 'ARM7 Yamaha AICA 45 MHZ (with internal 32-bit RISC CPU, 64 channel ADPCM)',
      ram: '40 MB',
      media: 'ROM Board (max. size of 168 MB)',
    },
    colors: {
      eu: {
        band1: '#C11127',
        band2: '#FF6B00',
        band3: '#04EF94',
        band4: '#029205',
      },
    },
  },
  bbcmicro: {
    system: {
      manufacturer: 'Acorn Computers',
      yearOfRelease: '1981',
      os: 'Acorn MOS',
      cpu: 'MOS Technology 6502/6512 @ 2 MHz',
      ram: '64KB - 128KB',
      drive: "Cassette tape, Floppy disk 5¼ or 3½, Hard disk also known as 'Winchester' (Rare), Laserdisc (BBC Domesday Project)",
      soundChip: 'Texas Instruments SN76489, 4 channels, mono',
      resolution: '640x256, 8 colours (various framebuffer modes)',
    },
    colors: {
      eu: {
        band1: '#DAD6CA',
        band2: '#BDB7A7',
        band3: '#EE7670',
        band4: '#444544',
      },
    },
  },
  bk: {
    system: {
      manufacturer: 'Elektronika',
      yearOfRelease: '1984',
      os: 'Focal',
      cpu: 'K1801VM1 @ 3MHz',
      ram: '32 KB',
      rom: '8 KB',
      gpu: 'K1801VP1-037',
      soundChip: 'Covox DACs',
      resolution: '512x256 (monochrome) / 256x256 (4 colors)',
    },
    colors: {
      eu: {
        band1: '#DDD6BA',
        band2: '#CABFA3',
        band3: '#231F20',
        band4: '#861719',
      },
    },
  },
  c64: {
    system: {
      manufacturer: 'Commodore',
      yearOfRelease: '1982',
      os: 'Commodore KERNAL / Commodore BASIC 2.0 GEOS',
      cpu: 'MOS Technology 6510/8500 @ 1.023 MHz (NTSC version)',
      ram: '64KB RAM',
      rom: '20KB',
      gpu: 'MOS Technology VIC-II',
      soundChip: 'MOS Technology 6581 SID',
      resolution: '320x200 (16 colours)',
    },
    colors: {
      eu: {
        band1: '#BBAD93',
        band2: '#968971',
        band3: '#FD4120',
        band4: '#00A0C6',
      },
    },
  },
  cdi: {
    system: {
      manufacturer: 'Philips, Magnavox',
      yearOfRelease: '1991',
      os: 'CD-RTOS (Compact Disc Real Time Operating System)',
      cpu: '16-bit CISC (680070) running @ 15.5 MHz',
      ram: '1 MB / VRAM',
      rom: '512 KB',
      gpu: 'SCC66470, later MCD 212',
      soundChip: '16-bit Stereo Sound with 44.1 KHz Sound Sampling Rate',
      resolution: '384x280 to 768x560 (16.7 million colours)',
    },
    colors: {
      eu: {
        band1: '#6b6d65',
        band2: '#f7f8f5',
        band3: '#74666b',
        band4: '#0b5ed8',
      },
    },
  },
  channelf: {
    system: {
      manufacturer: 'Fairchild Semiconductor',
      yearOfRelease: '1976',
      unitsSold: '250000',
      cpu: 'Fairchild F8 @ 1.79 MHz (PAL 2.22 MHz)',
      ram: '64 bytes',
      vram: '2 kB',
      resolution: '128x64, 8 colors',
      sound: '500 Hz, 1 kHz and 1.5 kHz tones (can be modulated)',
      media: 'ROM Cartridge',
    },
    colors: {
      eu: {
        band1: '#E0E0E0',
        band2: '#91FFA6',
        band3: '#CED0FF',
        band4: '#4B3FF3',
      },
    },
  },
  chihiro: {
    system: {
      manufacturer: 'Microsoft, Sega',
      cpu: 'Intel Pentium III @ 733 MHz',
      gpu: 'Nvidia XChip, GeForce 3 based @ 200 MHz',
      ram: '128MB maximum',
    },
    colors: {
      eu: {
        band1: '#',
        band2: '#',
        band3: '#',
        band4: '#',
      },
    },
  },
  colecovision: {
    system: {
      manufacturer: 'Coleco',
      yearOfRelease: '1982',
      unitsSold: '2+ million',
      bestSellingGame: 'Donkey Kong',
      cpu: 'Zilog Z80 @ 3.58 MHz',
      ram: '1kB / VRAM',
      video: 'Texas Instruments TMS9928A',
      resolution: '256x192, 16 colors',
      soundChip: 'Texas Instruments SN76489A',
      cartSize: '8kB - 32kB',
    },
    colors: {
      eu: {
        band1: '#EF8185',
        band2: '#F3994B',
        band3: '#FADD2F',
        band4: '#A4DDF0',
      },
    },
  },
  creativision: {
    system: {
      manufacturer: 'VTech',
      yearOfRelease: '1982',
      unitsSold: 'Unknown',
      numberGames: '18 officially released',
      cpu: '6502 @ 2MHz',
      ram: '1kB',
      video: 'TMS9928',
      resolution: '256x192, 16 colors',
      soundChip: 'Texas Instruments SN76489',
      cartSize: '4kB - 18kB',
    },
    colors: {
      eu: {
        band1: '#EFCB99',
        band2: '#9D5032',
        band3: '#73533E',
        band4: '#1F1F23',
      },
    },
  },
  daphne: {
    system: {
      developers: 'Matt Ownby (Daphne) and Jeffrey Clark (Hypseus)',
      yearOfRelease: '1999',
      website: 'http',
      writtenIn: 'C++',
      bestLaserdiscGamesAre: "Dragon's Lair, Space Ace, ...",
    },
    colors: {
      eu: {
        band1: '#DEBA27',
        band2: '#856125',
        band3: '#DC110B',
        band4: '#D8D3BD',
      },
    },
  },
  dice: {
    system: {
      yearOfRelease: '',
      website: '',
    },
    colors: {
      eu: {
        band1: '#B7151D',
        band2: '#00BAFF',
        band3: '#FF8200',
        band4: '#FFDBA1',
      },
    },
  },
  dos: {
    system: {
      manufacturer: 'IBM',
      yearOfRelease: '1981',
    },
    colors: {
      eu: {
        band1: '#1F70C1',
        band2: '#FFFFFF',
        band3: '#666870',
        band4: '#201D1D',
      },
    },
  },
  dragon: {
    system: {
      manufacturer: 'Dragon Data, Ltd. / Tano Corp.',
      yearOfRelease: '1982',
      os: 'Microsoft Extended BASIC',
      cpu: 'Motorola 6809E @ 0.89 MHz',
      ram: '32/64 KB',
      resolution: 'max 256 x 192, 8 colours',
      sound: '1 voice, 5 octaves with the Basic / 4 voices, 7 octaves with machine code',
    },
    colors: {
      eu: {
        band1: '#F50019',
        band2: '#FBFA35',
        band3: '#74B719',
        band4: '#3262D9',
      },
    },
  },
  dreamcast: {
    system: {
      manufacturer: 'Sega',
      yearOfRelease: '1998',
      unitsSold: '9.13 million',
      bestSellingGame: 'Sonic Adventure',
      cpu: 'Hitachi SH-4 32-bit RISC clocked @ 200 MHz',
      ram: '16MB / VRAM',
      video: "100 MHz PowerVR2 (integrated with the system's ASIC)",
      resolution: '640x480 pixels',
      soundChip: 'Yamaha AICA[139] sound processor @ 67 MHz',
      vmuSize: '128kB',
    },
    colors: {
      eu: {
        band1: '#C1C3C2',
        band2: '#3E649F',
        band3: '#030303',
        band4: '#E4E5E4',
      },
      jp: {
        band1: '#C1C3C2',
        band2: '#32507C',
        band3: '#CF3311',
        band4: '#E4E5E4',
      },
      us: {
        band1: '#C1C3C2',
        band2: '#32507C',
        band3: '#CF3311',
        band4: '#E4E5E4',
      },
    },
  },
  easyrpg: {
    system: {},
    colors: {
      eu: {
        band1: '#4D603D',
        band2: '#5F7057',
        band3: '#424241',
        band4: '#838382',
      },
    },
  },
  favorites: {
    system: {},
    colors: {
      eu: {
        band1: '#F6DD08',
        band2: '#800000',
        band3: '#FF0000',
        band4: '#4C504E',
      },
    },
  },
  fba: {
    system: {},
    colors: {
      eu: {
        band1: '#CF8A61',
        band2: '#FE7B07',
        band3: '#231F20',
        band4: '#444343',
      },
    },
  },
  fbneo: {
    system: {},
    colors: {
      eu: {
        band1: '#B7151D',
        band2: '#00BAFF',
        band3: '#FF8200',
        band4: '#FFDBA1',
      },
    },
  },
  fds: {
    system: {
      manufacturer: 'Nintendo',
      yearOfRelease: '1986',
      unitsSold: '4.44 million',
      bestSellingGame: 'Super Mario Bros. 2',
      cpu: 'Ricoh 2A03 8 Bit processor',
      ram: '32 Ko',
      diskSpace: '112 Ko',
      gpu: 'Uses Famicom hardware',
      soundChip: 'Uses Famicom hardware',
      ports: 'Uses Famicom hardware',
    },
    colors: {
      eu: {
        band1: '#A71636',
        band2: '#C7AF89',
        band3: '#3C3C3B',
        band4: '#F4B600',
      },
    },
  },
  gamecube: {
    system: {
      manufacturer: 'Nintendo',
      yearOfRelease: '2001',
      unitsSold: '21.74 million',
      bestSellingGame: 'Super Smash Bros. Melee',
      cpu: '128-bit IBM Gekko PowerPC @ 485 MHz',
      ram: '43 MB total non-unified RAM',
      video: '162 MHz "Flipper" LSI',
      resolution: '640x480 480i or 480p - 60 Hz',
      sound: '16-bit Dolby Pro Logic II',
      media: '1.5 GB miniDVD',
    },
    colors: {
      eu: {
        band1: '#7b79aa',
        band2: '#524c82',
        band3: '#cbc9e0',
        band4: '#1e1c1a',
      },
    },
  },
  gamegear: {
    system: {
      manufacturer: 'Sega',
      yearOfRelease: '1990',
      unitsSold: '11 million',
      cpu: '8-Bit Zilog Z80 @ 3.58 MHz',
      ram: '8kB / VRAM',
      video: 'Sega VDP (custom Texas Instruments TMS9918)',
      resolution: '160 x 146 pixels, 32 colors (4096 colors palette)',
      soundChip: 'TI SN76489 PSG',
      cartSize: '32kB - 1MB',
      screen: '8.128cm (3.2 inches), backlit LCD',
    },
    colors: {
      eu: {
        band1: '#CCCBCB',
        band2: '#D41808',
        band3: '#008200',
        band4: '#0000FE',
      },
    },
  },
  gb: {
    system: {
      manufacturer: 'Nintendo',
      yearOfRelease: '1989',
      unitsSold: '118.69 million',
      bestSellingGame: 'Tetris',
      cpu: '8-bit Zilog Z80 @ 4.194304MHz',
      ram: '8kB / VRAM',
      video: 'PPU (embedded in CPU)',
      resolution: '160x144 pixels, 4 shades of "gray"',
      sound: '4 channels stereo sound',
      cartSize: '32kB - 1MB',
    },
    colors: {
      eu: {
        band1: '#9B2063',
        band2: '#2D308E',
        band3: '#D9D9D9',
        band4: '#A3A3A3',
      },
    },
  },
  gba: {
    system: {
      manufacturer: 'Nintendo',
      yearOfRelease: '2001',
      unitsSold: '82 million',
      bestSellingGame: 'Pokemon Ruby',
      cpu: '16 MHz 32bit RISC-CPU + 8bit CISC-CPU',
      ram: '32KB WRAM + 256KB WRAM',
      screen: 'Reflective TFT Colour LCD',
      resolution: '240 x 160 pixels',
      videoRam: '256KB',
      displayAbility: '32 000 colours',
    },
    colors: {
      eu: {
        band1: '#5C67A9',
        band2: '#280FBE',
        band3: '#BCBCBC',
        band4: '#212121',
      },
    },
  },
  gbc: {
    system: {
      manufacturer: 'Nintendo',
      yearOfRelease: '1998',
      unitsSold: '118.69 million',
      bestSellingGame: 'Pokemon Gold and Silver',
      cpu: '8-bit Zilog Z80 @ 8.33 MHz',
      ram: '32kB / VRAM',
      video: 'PPU (embedded in CPU)',
      resolution: '160 x 144 pixels, 56 colors (32768 colors palette)',
      sound: '4 channels stereo sound',
      cartSize: '256kB - 8MB',
    },
    colors: {
      eu: {
        band1: '#F63B77',
        band2: '#2D308E',
        band3: '#00B3DE',
        band4: '#7642B6',
      },
    },
  },
  'genre-action': {
    system: {},
    colors: {
      eu: {
        band1: '#FD3135',
        band2: '#EB3D01',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-actionbattleroyale': {
    system: {},
    colors: {
      eu: {
        band1: '#F0020C',
        band2: '#830106',
        band3: '#FFFFFF',
        band4: '#000000',
      },
    },
  },
  'genre-actionbeatemup': {
    system: {},
    colors: {
      eu: {
        band1: '#FE5850',
        band2: '#FEAE50',
        band3: '#282828',
        band4: '#E8E9E8',
      },
    },
  },
  'genre-actionfighting': {
    system: {},
    colors: {
      eu: {
        band1: '#FFFFFF',
        band2: '#F41E22',
        band3: '#FDB915',
        band4: '#000000',
      },
    },
  },
  'genre-actionfirstpersonshooter': {
    system: {},
    colors: {
      eu: {
        band1: '#02A2C6',
        band2: '#04C453',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-actionplatformer': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#FFFFFF',
        band3: '#279400',
        band4: '#FFC705',
      },
    },
  },
  'genre-actionplatformshooter': {
    system: {},
    colors: {
      eu: {
        band1: '#E32B01',
        band2: '#B70170',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-actionrythm': {
    system: {},
    colors: {
      eu: {
        band1: '#04AE9E',
        band2: '#0C56A6',
        band3: '#7C07AB',
        band4: '#A9092E',
      },
    },
  },
  'genre-actionshootemup': {
    system: {},
    colors: {
      eu: {
        band1: '#2E01AB',
        band2: '#5F26FE',
        band3: '#000000',
        band4: '#8A8A8A',
      },
    },
  },
  'genre-actionshootwithgun': {
    system: {},
    colors: {
      eu: {
        band1: '#9F0108',
        band2: '#000000',
        band3: '#FFFFFF',
        band4: '#8A8A8A',
      },
    },
  },
  'genre-actionstealth': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#003821',
        band3: '#0E0567',
        band4: '#9A9A9A',
      },
    },
  },
  'genre-adventure': {
    system: {},
    colors: {
      eu: {
        band1: '#62F8EB',
        band2: '#A95DFD',
        band3: '#FE955C',
        band4: '#EFFF5B',
      },
    },
  },
  'genre-adventuregraphics': {
    system: {},
    colors: {
      eu: {
        band1: '#34C430',
        band2: '#343DC0',
        band3: '#CEC816',
        band4: '#000000',
      },
    },
  },
  'genre-adventureinteractivemovie': {
    system: {},
    colors: {
      eu: {
        band1: '#9A9A9A',
        band2: '#FFFFFF',
        band3: '#000000',
        band4: '#0104D3',
      },
    },
  },
  'genre-adventurerealtime3d': {
    system: {},
    colors: {
      eu: {
        band1: '#FC5E00',
        band2: '#FBDB01',
        band3: '#ADFE40',
        band4: '#000000',
      },
    },
  },
  'genre-adventuresurvivalhorror': {
    system: {},
    colors: {
      eu: {
        band1: '#FFFFFF',
        band2: '#000000',
        band3: '#BF010A',
        band4: '#8A8A8A',
      },
    },
  },
  'genre-adventuretext': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#7534B0',
        band3: '#FFFFFF',
        band4: '#8A8A8A',
      },
    },
  },
  'genre-adventurevisualnovels': {
    system: {},
    colors: {
      eu: {
        band1: '#FFFFFF',
        band2: '#F6062D',
        band3: '#000000',
        band4: '#FFAA00',
      },
    },
  },
  'genre-board': {
    system: {},
    colors: {
      eu: {
        band1: '#7A060E',
        band2: '#000000',
        band3: '#8A8A8A',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-casino': {
    system: {},
    colors: {
      eu: {
        band1: '#FFFFFF',
        band2: '#004802',
        band3: '#9B0B01',
        band4: '#000000',
      },
    },
  },
  'genre-casual': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#8D0199',
        band3: '#F690FE',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-compilation': {
    system: {},
    colors: {
      eu: {
        band1: '#23ABC1',
        band2: '#2330C1',
        band3: '#AC1BC9',
        band4: '#BD2731',
      },
    },
  },
  'genre-demoscene': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#9A9A9A',
        band3: '#201E1E',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-digitalcard': {
    system: {},
    colors: {
      eu: {
        band1: '#FFFFFF',
        band2: '#B20205',
        band3: '#000000',
        band4: '#8A8A8A',
      },
    },
  },
  'genre-educative': {
    system: {},
    colors: {
      eu: {
        band1: '#7BD7FF',
        band2: '#C07CFE',
        band3: '#FE8E7C',
        band4: '#FED87C',
      },
    },
  },
  'genre-party': {
    system: {},
    colors: {
      eu: {
        band1: '#C1B544',
        band2: '#AE976E',
        band3: '#479236',
        band4: '#174085',
      },
    },
  },
  'genre-pinball': {
    system: {},
    colors: {
      eu: {
        band1: '#F2F64E',
        band2: '#FE8B46',
        band3: '#000000',
        band4: '#9A9A9A',
      },
    },
  },
  'genre-puzzleandlogic': {
    system: {},
    colors: {
      eu: {
        band1: '#C1B544',
        band2: '#AE976E',
        band3: '#479236',
        band4: '#174085',
      },
    },
  },
  'genre-rpg': {
    system: {},
    colors: {
      eu: {
        band1: '#512B1B',
        band2: '#37A321',
        band3: '#188AEE',
        band4: '#000000',
      },
    },
  },
  'genre-rpgaction': {
    system: {},
    colors: {
      eu: {
        band1: '#C61302',
        band2: '#DB9001',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-rpgdungeoncrawler': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#8A8A8A',
        band3: '#FFFFFF',
        band4: '#054A00',
      },
    },
  },
  'genre-rpgfirstpersonpartybased': {
    system: {},
    colors: {
      eu: {
        band1: '#FEA4F1',
        band2: '#A4BFFE',
        band3: '#FFFFFF',
        band4: '#9A9A9A',
      },
    },
  },
  'genre-rpgjapanese': {
    system: {},
    colors: {
      eu: {
        band1: '#FFFFFF',
        band2: '#FE9CC8',
        band3: '#A4572E',
        band4: '#000000',
      },
    },
  },
  'genre-rpgmmo': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#1401BF',
        band3: '#0C6FB8',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-rpgtactical': {
    system: {},
    colors: {
      eu: {
        band1: '#8A8A8A',
        band2: '#2C5305',
        band3: '#1D9F6B',
        band4: '#000000',
      },
    },
  },
  'genre-simulation': {
    system: {},
    colors: {
      eu: {
        band1: '#FFFFFF',
        band2: '#000000',
        band3: '#9A9A9A',
        band4: '#6A6A6A',
      },
    },
  },
  'genre-simulationbuildandmanagement': {
    system: {},
    colors: {
      eu: {
        band1: '#43F7FB',
        band2: '#B140FE',
        band3: '#FFFFFF',
        band4: '#000000',
      },
    },
  },
  'genre-simulationfishandhunt': {
    system: {},
    colors: {
      eu: {
        band1: '#0C9AF8',
        band2: '#8A8A8A',
        band3: '#6D432D',
        band4: '#6DEF88',
      },
    },
  },
  'genre-simulationlife': {
    system: {},
    colors: {
      eu: {
        band1: '#1AE442',
        band2: '#F0C0E6',
        band3: '#9A9A9A',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-simulationscifi': {
    system: {},
    colors: {
      eu: {
        band1: '#01015F',
        band2: '#02395E',
        band3: '#000000',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-simulationvehicle': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#66B67A',
        band3: '#B860BC',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-sportcompetitive': {
    system: {},
    colors: {
      eu: {
        band1: '#B28100',
        band2: '#FFD157',
        band3: '#8A8A8A',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-sportfight': {
    system: {},
    colors: {
      eu: {
        band1: '#A90401',
        band2: '#000000',
        band3: '#FD340D',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-sportracing': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#FFFFFF',
        band3: '#FC180E',
        band4: '#8A8A8A',
      },
    },
  },
  'genre-sports': {
    system: {},
    colors: {
      eu: {
        band1: '#7EF89D',
        band2: '#7D81F9',
        band3: '#FB877B',
        band4: '#FACC7C',
      },
    },
  },
  'genre-sportsimulation': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#9A9A9A',
        band3: '#FFFFFF',
        band4: '#0F100E',
      },
    },
  },
  'genre-strategy': {
    system: {},
    colors: {
      eu: {
        band1: '#DC8FC3',
        band2: '#30DE99',
        band3: '#698CC6',
        band4: '#F62D35',
      },
    },
  },
  'genre-strategy4x': {
    system: {},
    colors: {
      eu: {
        band1: '#EFA92F',
        band2: '#9C5D26',
        band3: '#D01402',
        band4: '#000000',
      },
    },
  },
  'genre-strategyartillery': {
    system: {},
    colors: {
      eu: {
        band1: '#395D2F',
        band2: '#000000',
        band3: '#9A9A9A',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-strategyautobattler': {
    system: {},
    colors: {
      eu: {
        band1: '#FE8228',
        band2: '#4967DD',
        band3: '#000000',
        band4: '#9A9A9A',
      },
    },
  },
  'genre-strategymoba': {
    system: {},
    colors: {
      eu: {
        band1: '#000000',
        band2: '#BB0109',
        band3: '#9A9A9A',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-strategyrts': {
    system: {},
    colors: {
      eu: {
        band1: '#7C08D4',
        band2: '#020FDA',
        band3: '#000000',
        band4: '#8A8A8A',
      },
    },
  },
  'genre-strategytbs': {
    system: {},
    colors: {
      eu: {
        band1: '#41719B',
        band2: '#7132AA',
        band3: '#040404',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-strategytowerdefense': {
    system: {},
    colors: {
      eu: {
        band1: '#D11901',
        band2: '#000000',
        band3: '#D18001',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-strategywargame': {
    system: {},
    colors: {
      eu: {
        band1: '#FFE16B',
        band2: '#7EFE6C',
        band3: '#6CA2FE',
        band4: '#FFFFFF',
      },
    },
  },
  'genre-trivia': {
    system: {},
    colors: {
      eu: {
        band1: '#60D00C',
        band2: '#D80417',
        band3: '#4D0BD1',
        band4: '#000000',
      },
    },
  },
  gw: {
    system: {
      manufacturer: 'Nintendo',
      yearOfRelease: '1980',
      unitsSold: '43.4 million',
      bestSellingGame: 'Donkey Kong',
    },
    colors: {
      eu: {
        band1: '#C4353D',
        band2: '#C2904A',
        band3: '#853641',
        band4: '#4C301C',
      },
    },
  },
  gx4000: {
    system: {
      manufacturer: 'Amstrad',
      yearOfRelease: '1990',
      cpu: 'Zilog Z80A @ 4 MHz',
      ram: '64 KB',
      vram: '16 KB',
      rom: '32 KB',
      soundChip: 'General Instrument AY-3-8912, 3 channel stereo',
      resolution: '160x200 to 640x200, 16 sprites, 4096 colour palette - 32 onscreen',
    },
    colors: {
      eu: {
        band1: '#E5B3B2',
        band2: '#9E4968',
        band3: '#DBD9D4',
        band4: '#C02520',
      },
    },
  },
  imageviewer: {
    system: {},
    colors: {
      eu: {
        band1: '#F2F2F2',
        band2: '#D50F2A',
        band3: '#A19795',
        band4: '#4C504E',
      },
    },
  },
  intellivision: {
    system: {
      manufacturer: 'Mattel Electronics',
      yearOfRelease: '1979',
      cpu: 'GI CP1610 @ 894.886 kHz',
      ram: '1kB',
      video: 'General Instrument AY-3-8900-1',
      resolution: '159x96, 16 colors',
      soundChip: 'General Instrument AY-3-8914',
      cartSize: '8kB - 32kB',
    },
    colors: {
      eu: {
        band1: '#d4c1a0',
        band2: '#2b201d',
        band3: '#c81a7c',
        band4: '#c8d463',
      },
    },
  },
  jaguar: {
    system: {
      manufacturer: 'Atari Corporation',
      yearOfRelease: '1993',
      unitsSold: 'less than 250000',
      cpu: 'Five Main Processors in three chips',
      ram: '2MB',
      display: 'Max resolution 800x576 at 24 bit "true" color',
    },
    colors: {
      eu: {
        band1: '#ff0000',
        band2: '#fdc007',
        band3: '#232326',
        band4: '#0f0e0e',
      },
    },
  },
  kodi: {
    system: {},
    colors: {
      eu: {
        band1: '#186AAC',
        band2: '#36A2CD',
        band3: '#448F3E',
        band4: '#BF2E31',
      },
    },
  },
  lastplayed: {
    system: {},
    colors: {
      eu: {
        band1: '#AF774D',
        band2: '#96562B',
        band3: '#969BC7',
        band4: '#3678A6',
      },
    },
  },
  lightgun: {
    system: {},
    colors: {
      eu: {
        band1: '#C0BAC0',
        band2: '#BC4F41',
        band3: '#181913',
        band4: '#A3362A',
      },
    },
  },
  lowresnx: {
    system: {
      author: 'Timo Kloss',
      yearOfRelease: '2017',
      website: 'https',
      builtInFeatures: 'Gfx Designer for editing sprites, tiles, fonts and maps, as',
      display: '160x128 pixels, 60 Hz, 8 dynamic 6-bit palettes with 4 colors each',
      sprites: '64, max 32x32 pixels',
      sound: '4 voices, saw/tri/pulse/noise, pulse width, volume, ADSR, LFO',
    },
    colors: {
      eu: {
        band1: '#222222',
        band2: '#F15924',
        band3: '#F68920',
        band4: '#00AAA9',
      },
    },
  },
  lutro: {
    system: {},
    colors: {
      eu: {
        band1: '#74282b',
        band2: '#ae353a',
        band3: '#F2D8B5',
        band4: '#00007f',
      },
    },
  },
  lynx: {
    system: {
      manufacturer: 'Epyx / Atari',
      yearOfRelease: '1989',
      unitsSold: '1 million',
      cpu: '8-bit with 16-bit address space',
      cpu2: 'Mikey and Suzy 2 x 16-bit',
      ram: '64K 120 ns DRAM',
      rom: '512 bytes',
      resolution: '160 x 102 pixels',
      sound: '4 channels, 8-bit DAC',
    },
    colors: {
      eu: {
        band1: '#FD6420',
        band2: '#404040',
        band3: '#F38900',
        band4: '#262626',
      },
    },
  },
  macintosh: {
    system: {
      manufacturer: 'Apple Computer, Inc.',
      yearOfRelease: '1984',
      os: 'System Software 1.0',
      cpu: 'Motorola 68000 @ 7.8336 MHz',
      ram: '128 KB',
      rom: '64 KB',
      sound: '8 TTL chips implementing a video and sound DMA controller',
      display: '9" Monochrome built-in (512x342 pixels)',
    },
    colors: {
      eu: {
        band1: '#76B845',
        band2: '#E19433',
        band3: '#009DDD',
        band4: '#C45152',
      },
    },
  },
  mame: {
    system: {
      manufacturer: 'Nicola Salmoria & Mame Team',
      yearOfRelease: '1997',
      developers: 'Nicola Salmoria and the MAME Team',
      website: 'http',
      writtenIn: 'C++',
    },
    colors: {
      eu: {
        band1: '#00ADEF',
        band2: '#84849C',
        band3: '#231F20',
        band4: '#FFFFFF',
      },
    },
  },
  mastersystem: {
    system: {
      manufacturer: 'Sega',
      yearOfRelease: '1985',
      unitsSold: '13 million',
      bestSellingGame: 'Alex Kidd in Miracle World',
      cpu: '8-Bit Zilog Z80 @ 3.58 MHz',
      ram: '8kB / VRAM',
      video: 'Sega VDP (custom Texas Instruments TMS9918)',
      resolution: '256 x 192 pixels, 32 colors (64 colors palette)',
      soundChip: 'TI SN76489 PSG / add-on module YM2413 FM',
      cartSize: '32kB - 1MB',
    },
    colors: {
      eu: {
        band1: '#CBCBCB',
        band2: '#165193',
        band3: '#E60000',
        band4: '#1A1A1A',
      },
      jp: {
        band1: '#C6C6BB',
        band2: '#94A079',
        band3: '#1A82C5',
        band4: '#EED98E',
      },
    },
  },
  megadrive: {
    system: {
      manufacturer: 'Sega',
      yearOfRelease: '1988',
      unitsSold: '30 million',
      bestSellingGame: 'Sonic the Hedgehog',
      cpu: '16-bit Motorola 68000 @ 7.6 MHz',
      cpu2: '8-bit Zilog Z80 @ 3.58 Mhz (sound CPU)',
      ram: '64 kB / VRAM',
      video: '16-bit Sega VDP',
      resolution: '320x224 pixels, 64 colors (512 colors palette)',
      soundChip: 'Yamaha YM2612 FM and TI SN76489 PSG',
    },
    colors: {
      eu: {
        band1: '#DCE3E6',
        band2: '#884A52',
        band3: '#D4AA6F',
        band4: '#212122',
      },
      jp: {
        band1: '#D2D2D2',
        band2: '#A02B59',
        band3: '#0069B4',
        band4: '#00AA00',
      },
      us: {
        band1: '#181C4E',
        band2: '#811B15',
        band3: '#D2D2D2',
        band4: '#20211D',
      },
    },
  },
  megaduck: {
    system: {
      manufacturer: 'Welback Holdings',
      yearOfRelease: '1993',
      cpu: 'MOS version of the Z80 (embedded in the main VLSI)',
      ram: '16kB',
      resolution: '160x144 pixels, 4 shades of grey',
      sound: 'Built-in speaker (8Ω 200 mW)',
      media: '36 pins ROM cartridge',
    },
    colors: {
      eu: {
        band1: '#35b8c3',
        band2: '#77a6b0',
        band3: '#d7d8d7',
        band4: '#3f3f40',
      },
    },
  },
  model3: {
    system: {},
    colors: {
      eu: {
        band1: '#16A252',
        band2: '#EB1758',
        band3: '#236EBC',
        band4: '#F7ED18',
      },
    },
  },
  moonlight: {
    system: {
      createdBy: 'Case Western Reserve University students',
      portedOnLinuxBy: 'irtimmer',
      type: 'Open Source implementation',
      yearOfRelease: '2013',
      pcCompatibility: 'GFA PC with GTX 600/700/900/ ... GPU',
      necessarySoft: 'Nvidia GeForce Experience',
    },
    colors: {
      eu: {
        band1: '#82BD0F',
        band2: '#003366',
        band3: '#A4A1A4',
        band4: '#DEDDDE',
      },
    },
  },
  msx1: {
    system: {
      manufacturer: 'Various',
      yearOfRelease: '1983',
      os: 'MSX BASIC V1.0 (16 kB)',
      cpu: 'Zilog Z80A running @ 3.58 MHz',
      ram: '8KB to 128KB / VRAM',
      rom: '32KB',
      gpu: 'Texas Instruments TMS9918 family',
      soundChip: 'General Instrument AY-3-8910 (PSG)',
      resolution: '256x192 (16 colours)',
    },
    colors: {
      eu: {
        band1: '#9d9b91',
        band2: '#89283b',
        band3: '#3c5e43',
        band4: '#2761a2',
      },
    },
  },
  msx2: {
    system: {
      manufacturer: 'Various',
      yearOfRelease: '1985',
      os: 'MSX BASIC V3.0 (16 kB)',
      cpu: 'Zilog Z80 compatible @ 3.58 MHz',
      ram: '64 KB / VRAM',
      rom: '64 KB',
      gpu: 'Yamaha V9958 (aka MSX-Video)',
      soundChip: 'Yamaha YM2149 (PSG)',
      resolution: '256x212 (19268 simultaneous colors)',
    },
    colors: {
      eu: {
        band1: '#d6d4d4',
        band2: '#b7b7b7',
        band3: '#7c3036',
        band4: '#e3cf94',
      },
    },
  },
  msxturbor: {
    system: {
      manufacturer: 'Panasonic',
      yearOfRelease: '1990',
      os: 'MSX BASIC V4.0 (32 kB)',
      cpu: 'R800 (DAR800-X0G) @ 28.64 MHz and Z80A @ 3.58',
      ram: '256 KB (FS-A1ST) or 512 KB (FS-A1GT) / VRAM',
      rom: '96 KB',
      gpu: 'Yamaha V9958',
      soundChip: 'PSG (AY-3-8910 compatible), MSX-MUSIC, PCM, MSX-MIDI',
      resolution: '512 x 212 (4 or 16 colors) and 256 x 212 (16 to 19268 colors)',
    },
    colors: {
      eu: {
        band1: '#637080',
        band2: '#28292b',
        band3: '#99784b',
        band4: '#32100f',
      },
    },
  },
  multiplayer: {
    system: {},
    colors: {
      eu: {
        band1: '#F3F1A5',
        band2: '#679EA3',
        band3: '#8A9667',
        band4: '#93627F',
      },
    },
  },
  multivision: {
    system: {
      manufacturer: 'Tsukuda Original',
      yearOfRelease: '1983',
      cpu: 'Zilog Z80 @ 3.58 MHz',
      ram: '1KB / VRAM',
      video: 'Texas Instruments TMS9918',
      resolution: '256x192 pixels, 16 colors (21 colors palette)',
      soundChip: 'Texas Instruments SN76489 PSG',
    },
    colors: {
      eu: {
        band1: '#efebe1',
        band2: '#571229',
        band3: '#183b67',
        band4: '#e8e342',
      },
    },
  },
  n64: {
    system: {
      manufacturer: 'Nintendo',
      yearOfRelease: '1996',
      unitsSold: '32.93 million',
      bestSellingGame: 'Super Mario 64',
      cpu: '64-bit NEC VR4300 @ 93.75 MHz',
      cpu2: '64-bit MIPS RISC "Reality Immersion" RCP @ 62.5 MHz',
      ram: '4 MB RAMBUS RDRAM (expandable up to 8 MB)',
      sound: 'Stereo 16-Bit and 48 kHz',
    },
    colors: {
      eu: {
        band1: '#233387',
        band2: '#238B41',
        band3: '#C33D3A',
        band4: '#E6B531',
      },
    },
  },
  naomi: {
    system: {
      manufacturer: 'Sega',
      yearOfRelease: '1998',
      cpu: 'Hitachi SH-4 32-bit RISC CPU (@ 200 MHz 360 MIPS / 1.4 GFLOPS)',
      gpu: 'NEC-Videologic PowerVR (PVR2DC/CLX2) @ 100 MHz',
      soundChip: 'ARM7 Yamaha AICA 45 MHZ (with internal 32-bit RISC CPU, 64 channel ADPCM)',
      ram: '56 MB (64 MB with GD-ROM)',
      media: 'ROM Board (max. size of 168 MB) / GD-ROM',
    },
    colors: {
      eu: {
        band1: '#E4E7D7',
        band2: '#FD95B0',
        band3: '#48B3EB',
        band4: '#35FF90',
      },
    },
  },
  naomi2: {
    system: {
      manufacturer: 'Sega',
      yearOfRelease: '2000',
      cpu: 'Hitachi SH-4 32-bit RISC CPU (@ 200 MHz 360 MIPS / 1.4 GFLOPS)',
      gpu: 'NEC-Videologic PowerVR2 (CLX2)',
      soundChip: 'ARM7 Yamaha AICA 45 MHz (with internal 32-bit RISC CPU, 64 channels ADPCM)',
      ram: '56 MB',
      media: 'ROM Board (max. size of 168 MB)',
    },
    colors: {
      eu: {
        band1: '#F36418',
        band2: '#070308',
        band3: '#4F556B',
        band4: '#0057F0',
      },
    },
  },
  naomigd: {
    system: {
      manufacturer: 'Sega',
      yearOfRelease: '1998',
      cpu: 'Hitachi SH-4 32-bit RISC CPU (@ 200 MHz 360 MIPS / 1.4 GFLOPS)',
      gpu: 'NEC-Videologic PowerVR (PVR2DC/CLX2) @ 100 MHz',
      soundChip: 'ARM7 Yamaha AICA 45 MHZ (with internal 32-bit RISC CPU, 64 channel ADPCM)',
      ram: '56 MB (64 MB with GD-ROM)',
      media: 'ROM Board (max. size of 168 MB) / GD-ROM',
    },
    colors: {
      eu: {
        band1: '#E4E7D7',
        band2: '#A39B7C',
        band3: '#C75433',
        band4: '#243551',
      },
    },
  },
  nds: {
    system: {
      manufacturer: 'Nintendo',
      yearOfRelease: '2004',
      unitsSold: '153.99 million',
      bestSellingGame: 'New Super Mario Bros.',
      cpu: '66 MHz ARM9 + 33MHz ARM7',
      ram: '4MB / Video RAM',
      screen: 'Reflective TFT Colour LCD',
      resolution: '256 x 192 pixels',
      displayAbility: '260 000 colours',
      sound: 'Stereo speakers w/virtual surround, headphones',
    },
    colors: {
      eu: {
        band1: '#5C67A9',
        band2: '#280FBE',
        band3: '#BCBCBC',
        band4: '#212121',
      },
    },
  },
  neogeo: {
    system: {
      manufacturer: 'SNK Corporation',
      yearOfRelease: '1990',
      unitsSold: '1 million',
      bestSellingGame: "The King of Fighters '95",
      cpu: 'Motorola 68000 @ 12MHz, Zilog Z80A @ 4MHz',
      ram: '64KB RAM',
      audioRam: '2KB',
      videoRam: '84KB',
      display: '320x224 resolution, 4096 colors out of 65536',
      soundChip: 'Yamaha YM2610',
    },
    colors: {
      eu: {
        band1: '#BF9328',
        band2: '#494949',
        band3: '#2D2D2D',
        band4: '#E8E2E9',
      },
    },
  },
  neogeocd: {
    system: {
      manufacturer: 'SNK Corporation',
      yearOfRelease: '1990',
      unitsSold: '570000',
      cpu: '16-bit Motorola 68000 @ 12MHz, 8-bit Zilog Z80 @ 4MHz',
      ram: '7 MB RAM',
      sram: '2 KB',
      videoRam: '512 KB',
      soundChip: 'Yamaha YM2610 with 13 channels',
      display: '304x224 resolution, 4096 colors out of 65536',
    },
    colors: {
      eu: {
        band1: '#74756E',
        band2: '#FFD400',
        band3: '#0088CD',
        band4: '#24241E',
      },
    },
  },
  nes: {
    system: {
      manufacturer: 'Nintendo',
      yearOfRelease: '1983',
      unitsSold: '61.79 million',
      bestSellingGame: 'Super Mario Bros.',
      cpu: '8-bit MOS 6502 @ 1.79MHz',
      ram: '2kB / VRAM',
      video: '8-Bit PPU (Picture Processing Unit)',
      resolution: '256 x 240 pixels, 25 colors (64 colors palette)',
      sound: 'PSG sound (2 Square Waves, 1 Triangle Wave, 1 White Noise)',
      cartSize: '8kB - 1MB',
    },
    colors: {
      eu: {
        band1: '#C23435',
        band2: '#E0E0E1',
        band3: '#1A1919',
        band4: '#818080',
      },
      jp: {
        band1: '#EA2C27',
        band2: '#E6E7E8',
        band3: '#C0A987',
        band4: '#AA1C41',
      },
      us: {
        band1: '#C23435',
        band2: '#E0E0E1',
        band3: '#1A1919',
        band4: '#818080',
      },
    },
  },
  ngp: {
    system: {
      manufacturer: 'SNK',
      yearOfRelease: '1999',
      unitsSold: '2 million, including color version',
      cpu: 'Toshiba TLCS900H @ 6.144 MHz, Z80 @ 3.072 MHz',
      ram: '64 Kbytes',
      gpu: '16-bit',
      resolution: '160x152 (256x256 virtual screen)',
      media: 'ROM cartridge 4 MB max',
    },
    colors: {
      eu: {
        band1: '#B2B8BD',
        band2: '#E92A30',
        band3: '#25374A',
        band4: '#0F1520',
      },
    },
  },
  ngpc: {
    system: {
      manufacturer: 'SNK',
      yearOfRelease: '1999',
      unitsSold: '2 million, including no color version',
      cpu: 'Toshiba TLCS900H @ 6.144 MHz, Z80 @ 3.072 MHz',
      ram: '64 Kbytes',
      gpu: '16-bit',
      resolution: '160x152 146 colors (256x256 virtual screen)',
      media: 'ROM cartridge 4 MB max',
    },
    colors: {
      eu: {
        band1: '#E92A30',
        band2: '#18A94E',
        band3: '#1D9ED8',
        band4: '#25374A',
      },
    },
  },
  o2em: {
    system: {
      manufacturer: 'Magnavox/Philips',
      yearOfRelease: '1978',
      cpu: 'Intel 8048 @ 1.79 MHz',
      ram: '64 bytes',
      sound: '1 channel, 8 sounds',
      rom: '1024 bytes',
      resolution: '160×200 pixels with 12 colors',
    },
    colors: {
      eu: {
        band1: '#8C1914',
        band2: '#89918F',
        band3: '#DC3114',
        band4: '#F8D239',
      },
    },
  },
  openbor: {
    system: {
      developers: 'Senile Team, OpenBOR Team and the Community',
      yearOfRelease: '2004',
      website: 'http',
      writtenIn: 'C, C++',
    },
    colors: {
      eu: {
        band1: '#E17930',
        band2: '#407AC0',
        band3: '#EBE470',
        band4: '#404040',
      },
    },
  },
  oricatmos: {
    system: {
      developers: 'Tangerine Computer Systems',
      manufacturer: 'Tangerine Computer Systems',
      os: 'Tangerine/Microsoft Extended Basic v1.0',
      yearOfRelease: '1982',
      unitsSold: '210000',
      cpu: 'MOS 6502A @ 1 MHz',
      memory: '16 KB / 48 KB',
      storage: 'Tape recorder, 300 and 2400 baud',
      graphics: '40x28 text characters/ 240x200 pixels, 8 colours',
    },
    colors: {
      eu: {
        band1: '#BFB8A8',
        band2: '#207EA4',
        band3: '#77776D',
        band4: '#9E9C8F',
      },
    },
  },
  p2000t: {
    system: {
      manufacturer: 'Philips',
      yearOfRelease: '1981',
      os: 'BASIC',
      cpu: 'Zilog Z80 @ 2.5 MHz',
      ram: '16KB RAM (8KB for video) (16KB of system memory up to 48KB)',
      rom: '4KB',
      gpu: 'Mullard SAA5050 Teletext chip',
      soundChip: 'Beeper, 1 channel',
      resolution: '40 x 24 characters',
    },
    colors: {
      eu: {
        band1: '#9696A0',
        band2: '#1D1C21',
        band3: '#B0BC4E',
        band4: '#BD8254',
      },
    },
  },
  palm: {
    system: {
      manufacturer: 'Palm, Inc.',
      yearOfRelease: '2002',
      os: 'Palm OS version 4.1',
      cpu: 'Motorola DragonBall VZ MC68VZ328 @ 33 MHz',
      ram: '16 MB RAM',
      rom: '4 MB Flash',
      display: '160x160 pixel (65,000+ colors)',
    },
    colors: {
      eu: {
        band1: '#B2AFAC',
        band2: '#242443',
        band3: '#345A89',
        band4: '#363636',
      },
    },
  },
  pc88: {
    system: {
      manufacturer: 'NEC Corporation',
      yearOfRelease: '1981',
      os: 'N88-BASIC, N-BASIC (PC8001 MODE)',
      cpu: 'µPD780c-1 (Z80A-compatible) @ 4 MHz',
      ram: '64 KB (up to 576 KB) / VRAM',
      sound: '3 FM channels + 3 SSG + 6 rhythms + 1 ADPCM',
    },
    colors: {
      eu: {
        band1: '#B6A781',
        band2: '#342B22',
        band3: '#938CAA',
        band4: '#77B7C6',
      },
    },
  },
  pc98: {
    system: {
      manufacturer: 'NEC Corporation',
      yearOfRelease: '1982',
      os: 'CP/M-86, MS-DOS, OS/2, Windows',
      cpu: '8086 @ 5 MHz and higher',
      ram: '128 KB and higher',
      soundChip: 'Internal beeper, FM Sound, PCM',
    },
    colors: {
      eu: {
        band1: '#951610',
        band2: '#16643C',
        band3: '#00499A',
        band4: '#3B3431',
      },
    },
  },
  pcengine: {
    system: {
      manufacturer: 'NEC',
      yearOfRelease: '1987',
      unitsSold: '10 million',
      cpu: 'HuC6280 8-bit 7,16 Mhz',
      ram: '8 Kbyte',
      gpu: '8-bit',
      videoRam: '64 Kbyte',
      media: 'HU-Card, SHU-Card, CD-ROM, Super-CD-ROM',
    },
    colors: {
      eu: {
        band1: '#B2B2B2',
        band2: '#FE0000',
        band3: '#333333',
        band4: '#D9D9D9',
      },
      us: {
        band1: '#F79226',
        band2: '#ECF332',
        band3: '#FD5A17',
        band4: '#D5E14D',
      },
    },
  },
  pcenginecd: {
    system: {
      manufacturer: 'NEC',
      yearOfRelease: '1987',
      unitsSold: '10 million',
      cpu: 'HuC6280 8-bit 7,16 Mhz',
      ram: '8 Kbyte',
      gpu: '8-bit',
      videoRam: '64 Kbyte',
      media: 'HU-Card, SHU-Card, CD-ROM, Super-CD-ROM',
    },
    colors: {
      eu: {
        band1: '#B2B2B2',
        band2: '#FE0000',
        band3: '#333333',
        band4: '#D9D9D9',
      },
      us: {
        band1: '#F79226',
        band2: '#D5E14D',
        band3: '#604832',
        band4: '#1E1C1A',
      },
    },
  },
  pcfx: {
    system: {
      manufacturer: 'NEC Home Electronics',
      yearOfRelease: '1994',
      unitsSold: '400000',
      cpu: '32-bit NEC V810 RISC @ 21.5 MHz',
      ram: '2 MB (32 KB backup)',
      vram: '1.25 MB',
      sound: '16-bit stereo with ADPCM',
      resolution: '640x480 pixels',
      media: 'CD-ROM',
    },
    colors: {
      eu: {
        band1: '#FEC101',
        band2: '#01015B',
        band3: '#5C3073',
        band4: '#F00020',
      },
    },
  },
  pcv2: {
    system: {
      manufacturer: 'Benesse Corporation',
      yearOfRelease: '2000',
      cpu: '16Bits NEC V20',
      ram: '512k video',
      resolution: '224 x 144 pixels',
      sound: '4 PCM channels, 32 samples 4Bit for each channel',
    },
    colors: {
      eu: {
        band1: '#92AA6C',
        band2: '#E7D7B8',
        band3: '#487AB1',
        band4: '#3B3D53',
      },
    },
  },
  pico: {
    system: {
      manufacturer: 'Sega',
      yearOfRelease: '1993',
      cpu: '16-bit Motorola 68000 @ 7.6 MHz',
      resolution: '320x224 pixels NTSC, 320x240 PAL',
    },
    colors: {
      eu: {
        band1: '#6C378B',
        band2: '#25BCC1',
        band3: '#FAC367',
        band4: '#DB324C',
      },
    },
  },
  pico8: {
    system: {
      developers: 'Lexaloffle Games',
      yearOfRelease: '2015',
      website: 'www.pico-8.com',
      builtInFeatures: 'Lua code editor, sprite and map creation tools, and an audio sound effect and music editor.',
      display: '128x128 pixels, 16-color palette',
      sprites: '256 8x8 foreground sprites, 128x32 cels map',
      sound: '4 channel chip blerps',
    },
    colors: {
      eu: {
        band1: '#ff004d',
        band2: '#ffec27',
        band3: '#29adff',
        band4: '#ff77a8',
      },
    },
  },
  pokemini: {
    system: {
      manufacturer: 'Nintendo',
      yearOfRelease: '2001',
      bestSellingGame: 'Pokemon Party mini',
      cpu: 'S1C88 @ 4 MHz',
      ram: '4 kB (shared with video subsystem)',
      display: '96 x 64 pixels monochrome LCD',
      sound: 'Single channel PWM sound with three volume levels.',
      cartSize: '512 kB',
    },
    colors: {
      eu: {
        band1: '#FCE200',
        band2: '#004F8A',
        band3: '#999998',
        band4: '#8C8C8C',
      },
    },
  },
  ports: {
    system: {},
    colors: {
      eu: {
        band1: '#FF69B4',
        band2: '#36D7B7',
        band3: '#4B77BE',
        band4: '#BE90D4',
      },
    },
  },
  ps2: {
    system: {
      manufacturer: 'Sony',
      yearOfRelease: '2000',
      unitsSold: '158 million',
      bestSellingGame: 'Grand Theft Auto',
      cpu: 'R5900-based "Emotion Engine" @ 294-299 MHz',
      gpu: '"Graphics Synthesizer" @ 149 MHz',
      ram: '32 MB PC800 32-bit dual-channel RDRAM @ 400 MHz',
      videoRam: '4 MB',
      resolution: '256x224 to 1920x1080',
      media: 'DVD, CD',
    },
    colors: {
      eu: {
        band1: '#33438A',
        band2: '#0098DF',
        band3: '#40A9A8',
        band4: '#849DCA',
      },
    },
  },
  ps3: {
    system: {
      manufacturer: 'Sony, Foxconn, Asus',
      yearOfRelease: '2006',
      unitsSold: '83.8 million',
      bestSellingGame: 'Grand Theft Auto V',
      cpu: 'Cell Broadband Engine @ 3.2 GHz',
      gpu: 'RSX "Reality Synthesizer" @ 500MHz',
      ram: '256MB XDR @ 3.2 GHz',
      videoRam: '256MB GDDR3 @ 700 MHz',
      resolution: '480i, 480p, 720p, 1080i, 1080p',
      media: 'Blu-Ray, DVD, Compact Disc, PlayStation 1 and 2 discs, Super Audio CD',
    },
    colors: {
      eu: {
        band1: '#94B8F2',
        band2: '#1A15FF',
        band3: '#0095D5',
        band4: '#3DD3C8',
      },
    },
  },
  ps4: {
    system: {
      manufacturer: 'Sony',
      yearOfRelease: '2013',
      unitsSold: '106 millions',
      bestSellingGame: 'Marvel\'s Spider-Man',
      cpu: 'Semi-custom 8-core AMD x86-64 Jaguar 1.6 GHz CPU',
      gpu: 'Custom AMD GCN 18 CUs 800 MHz 1.84 TFLOPS',
      ram: '8 GB GDDR5 RAM',
      storage: 'HDD and SDD 500 GB, 1 TB, 2 TB',
    },
    colors: {
      eu: {
        band1: '#020202',
        band2: '#434342',
        band3: '#000000',
        band4: '#052352',
      },
    },
  },
  psp: {
    system: {
      manufacturer: 'Sony Computer Entertainment',
      yearOfRelease: '2004',
      unitsSold: '80.82 million',
      bestSellingGame: 'GTA Liberty City Stories',
      cpu: '32-bit MIPS RS4000 running at 333 Mhz',
      ram: '32 Mbits',
      dram: '4 Mbits',
      resolution: '480x272 px',
      size: '170x74x23 mm',
      weight: '260 g',
    },
    colors: {
      eu: {
        band1: '#313131',
        band2: '#0F1012',
        band3: '#97A9BA',
        band4: '#F9F7F7',
      },
    },
  },
  psvita: {
    system: {
      manufacturer: 'Sony',
      yearOfRelease: '2011',
      unitsSold: '10-15 millions',
      bestSellingGame: 'Uncharted: Golden Abyss',
      cpu: 'Quad-core ARM Cortex-A9 MPCore',
      gpu: 'Quad-core PowerVR SGX543MP4+',
      ram: '512 MB RAM, 128 MB VRAM',
      storage: '1 GB flash memory (PCH-2000 model only)',
    },
    colors: {
      eu: {
        band1: '#0F1012',
        band2: '#313131',
        band3: '#1E398E',
        band4: '#F5FAFE',
      },
    },
  },
  psx: {
    system: {
      manufacturer: 'Sony Computer Entertainment',
      yearOfRelease: '1994',
      unitsSold: '102.48 million',
      bestSellingGame: 'Gran Turismo',
      cpu: '32-bit R3000A RISC running at 33.8688 Mhz',
      ram: '16 Mbits',
      videoRam: '8 Mbits',
      audioRam: '4 Mbits',
      gpu: 'GFX processor unit, 32bit',
    },
    colors: {
      eu: {
        band1: '#DC0029',
        band2: '#F1C002',
        band3: '#00A79B',
        band4: '#326BB1',
      },
    },
  },
  samcoupe: {
    system: {
      developers: 'Miles Gordon Technology',
      manufacturer: 'Miles Gordon Technology',
      os: 'SAM BASIC',
      yearOfRelease: '1989',
      unitsSold: '12000',
      cpu: 'Zilog Z80B @ 6 MHz',
      memory: '256 KB/512 KB (4.5 MB max.)',
      storage: 'Cassette recorder, 3.5 inch floppy disk',
      graphics: '4 Modes (256x192 / 512x192) and 128 colour palette',
    },
    colors: {
      eu: {
        band1: '#2A46A5',
        band2: '#C60000',
        band3: '#D3D8D3',
        band4: '#96958C',
      },
    },
  },
  satellaview: {
    system: {
      manufacturer: 'Nintendo',
      yearOfRelease: '1995',
      unitsSold: '2 million',
      media: 'BS-X Application Cartridge + optional 8MB Memory Pak',
      bsX: 'The Story of The Town Whose Name Was Stolen.',
    },
    colors: {
      eu: {
        band1: '#37336C',
        band2: '#CD2943',
        band3: '#FFDF68',
        band4: '#647A54',
      },
    },
  },
  saturn: {
    system: {
      manufacturer: 'Sega',
      yearOfRelease: '1994',
      unitsSold: '9.26 million',
      bestSellingGame: 'Virtua Fighter 2',
      cpu: '2x Hitachi SH-2 @ 28.6 MHz',
      ram: '16Mb / VRAM',
      video: 'VDP1 (sprite/texture and polygons), VDP2 (backgrounds)',
      resolution: '320x224 to 704x224, 16.77 million colors',
      soundChip: 'Motorola 68EC000, Yamaha FH1 YMF292, 32 channels',
      media: 'CD-ROM',
    },
    colors: {
      eu: {
        band1: '#0D7176',
        band2: '#D1AE44',
        band3: '#2740A5',
        band4: '#86254A',
      },
    },
  },
  scummvm: {
    system: {
      manufacturer: 'ScummVM Team',
      author: 'Ludvig Strigeus',
      yearOfRelease: '2001',
    },
    colors: {
      eu: {
        band1: '#EFD052',
        band2: '#C26317',
        band3: '#F9EDCB',
        band4: '#379742',
      },
    },
  },
  scv: {
    system: {
      manufacturer: 'Epoch',
      yearOfRelease: '1984',
      cpu: '8-bit NEC PD7801G',
      ram: '128KB / VRAM',
      video: 'EPOCH TV-1',
      resolution: '309x246 resolution, 16 colors, 128 on-screen sprites',
      soundChip: 'PD1771C @ 6 MHz',
    },
    colors: {
      eu: {
        band1: '#CFCFCE',
        band2: '#C02026',
        band3: '#39B54A',
        band4: '#E14B26',
      },
    },
  },
  sega32x: {
    system: {
      manufacturer: 'Sega',
      yearOfRelease: '1994',
      unitsSold: '665 000',
      cpu: 'Two 32-bit Hitachi SH2 @ 23 MHz + Megadrive CPUs',
      ram: '256kB / VRAM',
      video: '32X VDP',
      resolution: '320 x 224 pixels, 32768 colors',
      soundChip: 'Q-Sound 10bit Stereo PCM chip + Megadrive Sound chips',
      cartSize: '2MB - 4MB',
    },
    colors: {
      eu: {
        band1: '#DCE3E6',
        band2: '#D5202C',
        band3: '#C0C1C4',
        band4: '#212122',
      },
      jp: {
        band1: '#6177B3',
        band2: '#1D46A5',
        band3: '#F2BA5C',
        band4: '#FBE627',
      },
      us: {
        band1: '#0084E2',
        band2: '#ED0A0A',
        band3: '#FFE607',
        band4: '#212122',
      },
    },
  },
  segacd: {
    system: {
      manufacturer: 'Sega',
      yearOfRelease: '1991',
      unitsSold: '30 million',
      bestSellingGame: 'Sewer Shark',
      cpu: '16-bit Motorola 68000 @ 12.5 MHz + Megadrive CPUs',
      ram: '512kB / VRAM',
      video: 'Custom ASIC',
      resolution: '320x224 pixels, 64 colors (512 colors palette)',
      soundChip: 'Ricoh RF5C164 + Megadrive Sound chips',
      media: '500MB CD-ROM discs',
    },
    colors: {
      eu: {
        band1: '#DCE3E6',
        band2: '#D5202C',
        band3: '#C0C1C4',
        band4: '#212122',
      },
      jp: {
        band1: '#5A97D0',
        band2: '#1D46A5',
        band3: '#DEAD2C',
        band4: '#732A46',
      },
      us: {
        band1: '#4B95D1',
        band2: '#2B83C5',
        band3: '#B2AE85',
        band4: '#212122',
      },
    },
  },
  sg1000: {
    system: {
      manufacturer: 'Sega',
      yearOfRelease: '1983',
      unitsSold: '2 million',
      bestSellingGame: 'Flicky',
      cpu: '8-Bit Zilog Z80 @ 3.58 MHz',
      ram: '1kB / VRAM',
      video: 'Texas Instruments TMS9918',
      resolution: '256x192 pixels, 16 colors (21 colors palette)',
      soundChip: 'Texas Instruments SN76489 PSG',
      cartSize: '8kB - 48kB',
    },
    colors: {
      eu: {
        band1: '#D6A763',
        band2: '#9A2025',
        band3: '#000000',
        band4: '#BBBDBD',
      },
    },
  },
  snes: {
    system: {
      manufacturer: 'Nintendo',
      yearOfRelease: '1990',
      unitsSold: '49.10 million',
      bestSellingGame: 'Super Mario World',
      cpu: '16-bit Custom 65C816 @ 3.58 MHz',
      ram: '128kB / VRAM',
      video: '16-bit PPU (Picture Processor Unit)',
      resolution: '256x224, 256 colors (32768 colors palette)',
      soundChip: '8-bit Sony SPC700, 8 channels',
      cartSize: '256kB - 6MB',
    },
    colors: {
      eu: {
        band1: '#FED01B',
        band2: '#BA2318',
        band3: '#0A2A8D',
        band4: '#007544',
      },
      jp: {
        band1: '#F9EC2B',
        band2: '#01954B',
        band3: '#EB2D2F',
        band4: '#323B98',
      },
      us: {
        band1: '#A59EF7',
        band2: '#47449E',
        band3: '#CCCCCC',
        band4: '#4C4C4C',
      },
    },
  },
  solarus: {
    system: {
      manufacturer: 'Solarus Labs, Community',
      yearOfRelease: '2011',
    },
    colors: {
      eu: {
        band1: '#e5b131',
        band2: '#d37d2d',
        band3: '#574287',
        band4: '#3c6d35',
      },
    },
  },
  spectravideo: {
    system: {
      manufacturer: 'Spectravideo International',
      yearOfRelease: '1983',
      os: 'Microsoft Extended BASIC, CP/M',
      cpu: 'Zilog Z80A @ 3.58 MHz',
      rom: '32KB (16 KB BIOS / 16 KB BASIC)',
      ram: '16KB (SV-318) and 64Kb (SV-328) / VRAM',
      video: 'Texas Instruments TMS9918',
      resolution: '256x192, 16 colors',
      soundChip: 'General Instrument AY-3-8910 (PSG)',
      media: 'ROM Cartridge, Cassette tape',
    },
    colors: {
      eu: {
        band1: '#B74E6C',
        band2: '#C23435',
        band3: '#D17733',
        band4: '#E9E439',
      },
    },
  },
  sufami: {
    system: {
      manufacturer: 'Bandai',
      yearOfRelease: '1996',
      media: 'SuFami Turbo Application Cartridge +  Game Cartridges',
    },
    colors: {
      eu: {
        band1: '#FF0000',
        band2: '#EDDFDE',
        band3: '#A0C0CF',
        band4: '#1E2881',
      },
    },
  },
  supergrafx: {
    system: {
      manufacturer: 'NEC',
      yearOfRelease: '1989',
      unitsSold: '7,5 million',
      bestSellingGame: "Ghouls 'n Ghosts",
      cpu: 'HuC6280 8-bit 7,16 Mhz',
      ram: '8 Kbyte',
      gpu: '16-bit',
      videoRam: '64 Kbyte',
      media: 'HU-Card, SHU-Card, CD-ROM, Super-CD-ROM',
    },
    colors: {
      eu: {
        band1: '#b2b2b2',
        band2: '#0091ad',
        band3: '#000000',
        band4: '#D9D9D9',
      },
    },
  },
  supervision: {
    system: {
      manufacturer: 'Watara',
      yearOfRelease: '1992',
      cpu: '8-bit 65SC02 @ 4 MHz',
      ram: '8kB / VRAM',
      display: '160x160 pixels, 4 shades of grey',
      sound: '2 Tonal and 1 Noise Channel plus additional audio DMA stereo output',
    },
    colors: {
      eu: {
        band1: '#55C5CC',
        band2: '#64B0A6',
        band3: '#717A76',
        band4: '#68BD45',
      },
    },
  },
  switch: {
    system: {
      manufacturer: 'Nintendo',
      yearOfRelease: '2017',
      unitsSold: '',
      bestSellingGame: 'Mario Kart 8 Deluxe',
      cpu: 'ARM 4 Cortex-A57 cores @ 1.02 GHz',
      gpu: '256 Maxwell-based CUDA cores',
      ram: '4 GB LPDDR4 @ 1,331/1,600 MHz',
      storage: '32 GB / 64 GB eMMC',
    },
    colors: {
      eu: {
        band1: '#FF0000',
        band2: '#000000',
        band3: '#FF0000',
        band4: '#000000',
      },
    },
  },
  tate: {
    system: {},
    colors: {
      eu: {
        band1: '#C1B544',
        band2: '#6EAE97',
        band3: '#364792',
        band4: '#851740',
      },
    },
  },
  thomson: {
    system: {
      manufacturer: 'Thomson',
      yearOfRelease: '1986',
      cpu: 'Motorola 6809E @ 1 MHz',
      ram: '256kB, ROM',
      lowResolutions: '160 x 200 (16, 5 or 2 colors)',
      mediumResolutions: '320 x 200 (16, 4, 3 or 2 colors)',
      highResolutions: '640 x 200 (2 colors)',
      media: 'Cartridge, external 3½ floppy disk and tape drives',
    },
    colors: {
      eu: {
        band1: '#E6F0EF',
        band2: '#F0C73B',
        band3: '#4F8E5C',
        band4: '#2C2D2C',
      },
    },
  },
  ti994a: {
    system: {
      manufacturer: 'Texas Instruments',
      yearOfRelease: '1981',
      os: 'Basic owner',
      cpu: 'Texas Instruments TMS9900 @ 3.3 MHz',
      ram: '16/24 KB',
      resolution: 'max 256 x 192, 16 colours',
      sound: '3 voices on 5 octaves and noise channel',
    },
    colors: {
      eu: {
        band1: '#ffffff',
        band2: '#333333',
        band3: '#aa0404',
        band4: '#1a1a1a',
      },
    },
  },
  tic80: {
    system: {
      manufacturer: 'Vadim Grigoruk',
      author: 'Vadim Grigoruk',
      yearOfRelease: '2017',
      builtInFeatures: 'Code editor, Sprite Editor, Tile Map Editor, SFX Editor.',
      display: '240x136 pixels, 16-color palette',
      sprites: '256 8x8 foreground sprites, 256 8x8 background tiles',
      sound: '4 channels (with editable waveform envelopes)',
    },
    colors: {
      eu: {
        band1: '#4E76B4',
        band2: '#C5494A',
        band3: '#C7772F',
        band4: '#65A33D',
      },
    },
  },
  triforce: {
    system: {
      manufacturer: 'Namco, Nintendo, Sega',
      yearOfRelease: '2002',
      cpu: '128-bit IBM Gekko PowerPC @ 485 MHz',
      ram: '43 MB total non-unified RAM',
      video: '162 MHz "Flipper" LSI',
      resolution: '640x480 480i or 480p - 60 Hz',
      sound: '16-bit Dolby Pro Logic II',
      media: '1.5 GB miniDVD',
    },
    colors: {
      eu: {
        band1: '#',
        band2: '#',
        band3: '#',
        band4: '#',
      },
    },
  },
  trs80coco: {
    system: {
      manufacturer: 'Tandy Corporation',
      yearOfRelease: '1980',
      os: 'Microsoft Color Basic v1.0',
      cpu: 'MC6809E @ 0.89 MHz',
      ram: '4KB - 16KB',
      drive: 'Cassette tape, Floppy disk 5.25"',
      resolution: '32 Column Display Ability (32 x 16 with 4 Colors / 256 x 192 with 2 Colors / 128 x 96 at 8 Colors / 128 x 192 with 4 Colors)',
    },
    colors: {
      eu: {
        band1: '#B3B3B3',
        band2: '#CE2027',
        band3: '#0F7C3E',
        band4: '#3B509C',
      },
    },
  },
  uzebox: {
    system: {
      developers: 'Belogic Software',
      yearOfRelease: '2008',
      cpu: 'Amtel ATmega644 microcontroller @ 28.6 Mhz',
      ram: '4 K',
      programMemory: '64 K',
      sound: '5 channels wavetable synthesis , 8-bit mono, mixed at ~15Khz and output via PWM',
      resolution: 'Various video modes supporting up to 360x224 pixels in tiles-only mode',
      colors: '256 simultaneous colors arranged in a 3',
      media: 'SD Memory Card',
    },
    colors: {
      eu: {
        band1: '#C43F32',
        band2: '#EDC63D',
        band3: '#C9C2BF',
        band4: '#07336A',
      },
    },
  },
  vectrex: {
    system: {
      manufacturer: 'General Consumer Electronics',
      yearOfRelease: '1982',
      cpu: 'Motorola 68A09 @ 1.5 MHz',
      ram: '1KB (two 4-bit 2114 chips)',
      rom: '8KB (one 8-bit 2363 chip)',
      media: 'Cartridge with 32KB ROM',
      soundChip: 'General Instrument AY-3-8912',
    },
    colors: {
      eu: {
        band1: '#EEF020',
        band2: '#EC1D25',
        band3: '#11192E',
        band4: '#166FC1',
      },
    },
  },
  vg5000: {
    system: {
      manufacturer: 'Philips',
      yearOfRelease: '1984',
      os: 'Microsoft BASIC',
      cpu: 'Zilog Z80 @ 4 MHz',
      ram: '24KB RAM (8KB for video) (16KB of system memory up to 48KB)',
      rom: '18KB',
      gpu: 'SGS Thomson EF9345 (25 lines x 40 characters)',
      soundChip: '1 voice over 5 octaves',
      resolution: '320x250 (8 colours)',
    },
    colors: {
      eu: {
        band1: '#F2F5F7',
        band2: '#FFC000',
        band3: '#726A44',
        band4: '#242527',
      },
    },
  },
  vic20: {
    system: {
      manufacturer: 'Commodore',
      yearOfRelease: '1980',
      os: 'Commodore KERNAL / Commodore BASIC 2.0',
      cpu: 'MOS Technology 6502 @ 1.02 MHz (NTSC)/ 1.10 MHz (PAL)',
      ram: '5 KB RAM (expandable to 32 KB)',
      rom: '20KB',
      gpu: 'MOS Technology VIC-I 6561',
      soundChip: '3 x square, 1 x noise, mono',
      resolution: '128x128 (16 colours)',
    },
    colors: {
      eu: {
        band1: '#A07445',
        band2: '#4E4E4E',
        band3: '#E8EBD8',
        band4: '#B1BCBF',
      },
      jp: {
        band1: '#A07445',
        band2: '#B6A385',
        band3: '#E8EBD8',
        band4: '#9C8661',
      },
    },
  },
  videopacplus: {
    system: {
      manufacturer: 'Philips',
      yearOfRelease: '1983',
      cpu: 'Intel 8048 @ 5.91MHz',
      ram: '6 KB + 192 Bytes',
      sound: '1 channel, 8 sounds',
      rom: '1024 bytes',
      resolution: '320x238 pixels with 16 colors',
    },
    colors: {
      eu: {
        band1: '#F6C514',
        band2: '#EF6F4E',
        band3: '#E0286B',
        band4: '#BDB9A6',
      },
    },
  },
  vircon32: {
    system: {
      developers: 'Carra',
      yearOfRelease: '2023',
      cpu: '15 MHz CPU with floating point support',
      ram: '16 MB',
      display: 'Screen of 640 x 360 pixels (16/9) with true color',
      storage: '1 MB memory cards to save games',
      sound: 'CD-quality stereo sound with 16 channels',
    },
    colors: {
      eu: {
        band1: '#5CD48C',
        band2: '#04FCFC',
        band3: '#84B4FC',
        band4: '#64FC04',
      },
    },
  },
  virtualboy: {
    system: {
      manufacturer: 'Nintendo',
      yearOfRelease: '1995',
      unitsSold: '800 000',
      bestSellingGame: 'VB Wario Land',
      cpu: '32Bit NEC V810 RISC @ 20MHz',
      ram: '1Mbits DRAM 512kb P-SRAM',
      colors: '4 colors / 32 intensity levels',
      resolution: '384 x 224 pixels',
      screen: '2 RTI light-emitting LEDs',
      sound: '16Bits stereo',
    },
    colors: {
      eu: {
        band1: '#363636',
        band2: '#FE0016',
        band3: '#8A8C8E',
        band4: '#232323',
      },
    },
  },
  vpinball: {
    system: {
      yearOfRelease: '2000',
      website: 'vpforums.org',
      writtenIn: 'C++',
    },
    colors: {
      eu: {
        band1: '#0C0907',
        band2: '#EEE573',
        band3: '#EB332A',
        band4: '#D2CF1E',
      },
    },
  },
  wasm4: {
    system: {
      developers: 'Bruno Garcia',
      yearOfRelease: '2021',
      website: 'www.wasm4.org',
      ram: '64 KB linear RAM, memory-mapped I/O, save states',
      display: '60x160 pixels, 4 customizable colors, updated at 60 Hz',
      storage: 'Cartridge Size Limit 64 KB',
      sound: '2 pulse wave channels, 1 triangle wave channel, 1 noise channel',
      device: 'Keyboard, mouse, touchscreen, up to 4 gamepads',
    },
    colors: {
      eu: {
        band1: '#ffcd21',
        band2: '#e94f96',
        band3: '#6ebc87',
        band4: '#2a252e',
      },
    },
  },
  wii: {
    system: {
      developers: 'Nintendo',
      manufacturer: 'Foxconn',
      yearOfRelease: '2006',
      unitsSold: '101.63 million',
      bestSellingGame: 'Wii Sports',
      cpu: 'IBM PowerPC "Broadway" @ 729 MHz',
      ram: '88 MB',
      video: 'ATI "Hollywood" @ 243 MHz',
      resolution: '480i/p (PAL/NTSC)or 576i (PAL/SECAM), 4',
      sound: 'Stereo - Dolby Pro Logic II-capable',
      media: '4.7/8.54 GB DVD',
    },
    colors: {
      eu: {
        band1: '#24A9E2',
        band2: '#D1D1D1',
        band3: '#005C9A',
        band4: '#1E1C1A',
      },
    },
  },
  wiiu: {
    system: {
      manufacturer: 'Nintendo, Foxconn, Mitsumi',
      yearOfRelease: '2012',
      unitsSold: '13.56 million',
      bestSellingGame: 'Mario Kart 8',
      cpu: 'Tri-Core IBM PowerPC "Espresso" @ 1.24 GHz',
      gpu: 'AMD Radeon-based "Latte" @ 550 MHz',
      ram: '2 GB DDR3',
      sound: 'Dedicated 120 MHz audio processor',
      resolution: '480i, 480p, 720p, 1080i, 1080p',
      media: '25 GB Blu-ray based disc',
    },
    colors: {
      eu: {
        band1: '#1F9EBB',
        band2: '#3783BC',
        band3: '#DDDCDC',
        band4: '#9D8354',
      },
    },
  },
  wswan: {
    system: {
      manufacturer: 'Bandaï',
      yearOfRelease: '1999',
      cpu: '16Bits NEC V20',
      ram: '512k video',
      sound: '4 PCM channels, 32 samples 4Bit for each channel',
      resolution: '224 x 144 pixels',
    },
    colors: {
      eu: {
        band1: '#8797A4',
        band2: '#DBE0E1',
        band3: '#E72835',
        band4: '#282827',
      },
    },
  },
  wswanc: {
    system: {
      manufacturer: 'Bandaï',
      yearOfRelease: '2000',
      cpu: 'SSPGY-1002 @ 3.072 MHz 16-bit NEC V30MZ duplicate',
      ram: '64 kB shared VRAM/WRAM',
      sound: 'One mono speaker, optional headphones',
      resolution: '224 x 144 pixels',
    },
    colors: {
      eu: {
        band1: '#009CD4',
        band2: '#EBA743',
        band3: '#D65757',
        band4: '#162B4F',
      },
    },
  },
  x1: {
    system: {
      manufacturer: 'Sharp Corporation',
      yearOfRelease: '1982',
      os: 'CP/M',
      cpu: 'Sharp Z80 A running @ 4 MHz',
      ram: '64 KB / VRAM',
      rom: '6 KB',
      gpu: 'Sharp-Hudson Custom Chipset',
      soundChip: 'General Instrument AY-3-8910 or Yamaha YM2149',
      display: '320 x 200 / 640 x 200 (8 colours)',
    },
    colors: {
      eu: {
        band1: '#752421',
        band2: '#4A90CB',
        band3: '#C1C3D1',
        band4: '#28333D',
      },
    },
  },
  x68000: {
    system: {
      manufacturer: 'Sharp',
      yearOfRelease: '1987',
      os: 'Human68k',
      cpu: 'Motorola 68000 running @ 10 MHz',
      ram: '1-4 MB (expandable up to 12 MB) / VRAM',
      rom: '1 MB (128 KB BIOS, 768 KB Character Generator)',
      gpu: 'Sharp-Hudson Custom Chipset',
      soundChip: 'Yamaha YM2151',
      resolution: '512x512 (65536 colours)',
    },
    colors: {
      eu: {
        band1: '#2b3138',
        band2: '#686670',
        band3: '#C25F7F',
        band4: '#41BC83',
      },
    },
  },
  xbox: {
    system: {
      manufacturer: 'Microsoft',
      yearOfRelease: '2001',
      unitsSold: '24000000',
      cpu: 'Custom 733 MHz Intel Pentium III "Coppermine-based" processor',
      ram: '64 MB of DDR SDRAM @ 200 MHz',
      gpu: 'Nvidia GeForce 3-based NV2A GPU @ 233 MHz',
      sound: '16-bit stereo with ADPCM',
      media: 'DVD-ROM',
    },
    colors: {
      eu: {
        band1: '#93C83E',
        band2: '#000000',
        band3: '#FFFFFF',
        band4: '#75A22E',
      },
    },
  },
  xbox360: {
    system: {
      manufacturer: 'Microsoft',
      yearOfRelease: '2005',
      unitsSold: '85 millions',
      bestSellingGame: 'Kinect Adventures',
      cpu: '3.2 GHz PowerPC Tri-Core Xenon',
      gpu: '500 MHz ATI/AMD Xenos, 240 GFLOPS',
      ram: '512 MB of unified GDDR3 RAM clocked at 700 MHz, 10 MB of eDRAM cache on Xenos GPU',
    },
    colors: {
      eu: {
        band1: '#B4B7B9',
        band2: '#95C343',
        band3: '#B4B7B9',
        band4: '#95C343',
      },
    },
  },
  xboxone: {
    system: {
      manufacturer: 'Microsoft',
      yearOfRelease: '2013',
      unitsSold: '58 millions',
      bestSellingGame: 'Grand Theft Auto V',
      cpu: 'AMD Jaguar 8 cœurs x86 @ 1,75 GHz (One & One S) or AMD Custom 8 cœurs x86 2,3 GHz (One X)',
      gpu: 'AMD Radeon Graphics Core Next (AMD APU)',
      ram: '8 Go DDR3 2 @ 133 MHz (One & One S) or 12 Go GDDR5 (One X)',
    },
    colors: {
      eu: {
        band1: '#33903C',
        band2: '#AEAFAE',
        band3: '#77BF26',
        band4: '#E1E1E1',
      },
    },
  },
  zmachine: {
    system: {
      developers: 'Joel Berez and Marc Blank',
      yearOfRelease: '1979',
      device: 'Keyboard',
      display: '640x480 pixels',
    },
    colors: {
      eu: {
        band1: '#EEF5F8',
        band2: '#81F2CC',
        band3: '#3A3A3A',
        band4: '#488EAA',
      },
    },
  },
  zx81: {
    system: {
      developers: 'Sinclair Research',
      manufacturer: 'Timex Corporation',
      yearOfRelease: '1981',
      unitsSold: '1.5 million',
      cpu: 'Z80 at 3.25 MHz',
      memory: '1 KB (64 KB max. 56 KB usable)',
      storage: 'External cassette tape recorder at 250 baud',
      display: 'Monochrome display on UHF television',
      graphics: '24 lines x 32 characters or 64 x 48 pixels graphics mode',
    },
    colors: {
      eu: {
        band1: '#EF5857',
        band2: '#403E4B',
        band3: '#D2CECD',
        band4: '#191222',
      },
    },
  },
  zxspectrum: {
    system: {
      manufacturer: 'Sinclair Research',
      os: 'Sinclair BASIC',
      yearOfRelease: '1982',
      unitsSold: '5 million',
      cpu: 'Z80 @ 3.5 MHz and equivalent',
      memory: '16 KB / 48 KB / 128 KB',
      media: 'Cassette tape, 3-inch floppy disk on Spectrum +3',
    },
    colors: {
      eu: {
        band1: '#CA2825',
        band2: '#EDBA01',
        band3: '#009C37',
        band4: '#02559C',
      },
    },
  },
};
