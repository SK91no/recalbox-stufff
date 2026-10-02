/**
 * @author Pit64
 */
import { SimpleStringListObject } from 'stores/types/misc';

export interface CardReaderConfigResponse {
  consolemode: {
    exist: boolean;
    value: boolean;
  };
  consolemodeexit: {
    exist: boolean;
    value: string;
  };
  consolemodevideo: {
    exist: boolean;
    value: string;
  };
  consolemodesound: {
    exist: boolean;
    value: string;
  };
}

export interface CardReaderConfigOptionsResponse {
  consolemode: {
    type: string;
  };
  consolemodeexit: SimpleStringListObject;
  consolemodevideo: SimpleStringListObject;
  consolemodesound: SimpleStringListObject;
}
