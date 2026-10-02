/**
 * @author Nicolas TESSIER aka Asthonishia
 */

export interface VersionsResponse {
  webapi: string;
  recalbox: string;
  linux: string;
  libretro: {
    retroarch: string;
    cores: Record<string, string>;
  };
}
