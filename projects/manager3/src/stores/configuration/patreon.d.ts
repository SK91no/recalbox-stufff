/**
 * @author Pit64
 */

export interface PatreonConfigResponse {
  privatekey: {
    exist: boolean;
    value: string;
  };
}

export interface PatreonConfigOptionsResponse {
  privatekey: {
    type: string;
    allowedChars: string;
  };
}
