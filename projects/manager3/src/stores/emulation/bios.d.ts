/**
 * @author Nicolas TESSIER aka Asthonishia
 */

export interface Bios {
  system?: string;
  mandatory: boolean;
  hashMatchingMandatory: boolean;
  displayFileName: string;
  notes: string;
  currentMd5: string;
  md5List: string[];
  cores: string[];
  lightStatus: string;
  realStatus: string;
}

export type BiosResponse = Record<string, {
  fullName: string;
  scanResult: {
    total: number;
    ok: number;
    ko: number;
    unsafe: number;
    notFound: number;
    hashOk: number;
    hashKo: number;
  };
  biosList: Record<string, Bios>;
}>
