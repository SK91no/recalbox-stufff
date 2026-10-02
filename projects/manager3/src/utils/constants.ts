export enum Boards {
  ODROID_GO2= 'odroidgo2',
  ODROID_XU4 = 'odroidxu4',
  RG353X = 'rg353x',
  RPI_1 = 'rpi1',
  RPI_3 = 'rpi3',
  RPI_4_64 = 'rpi4_64',
  RPI_5_64 = 'rpi5_64',
  RPI_ZERO_2 = 'rpizero2',
  RPI_ZERO_2LEGACY = 'rpizero2legacy',
  X86_64 = 'x86_64',
}

export const BOARDS = [
  Boards.ODROID_XU4,
  Boards.RPI_1,
  Boards.RPI_3,
  Boards.RPI_4_64,
  Boards.RPI_5_64,
  Boards.RPI_ZERO_2,
  Boards.RPI_ZERO_2LEGACY,
  Boards.X86_64,
];

export const isBoard = (value: string): value is Boards => Object.values(Boards).includes(value as Boards);
