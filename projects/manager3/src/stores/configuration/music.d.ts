/**
 * @author Pit64
 */

export interface MusicConfigResponse {
  'remoteplaylist.enable': {
    exist: boolean;
    value: boolean;
  };
}

export interface MusicConfigOptionsResponse {
  'remoteplaylist.enable': {
    type: string;
  };
}
