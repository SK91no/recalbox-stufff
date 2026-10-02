import { quasarEsModulesPackageNames } from '@quasar/quasar-app-extension-testing-unit-jest/jest-preset.mjs';

/** @type {import('jest').Config} */
export default {
  preset: '@quasar/quasar-app-extension-testing-unit-jest',
  transform: {
    [`^(${quasarEsModulesPackageNames}).+\\.js$`]: 'babel-jest',
    '^.+\\.(ts|js|html)$': [
      'ts-jest',
      {
        // Remove if using `const enums`
        // See https://kulshekhar.github.io/ts-jest/docs/getting-started/options/isolatedModules/
        // isolatedModules: true,
      },
    ],
  },
  moduleNameMapper: {
    '^stores/(.+)$': '<rootDir>/src/stores/$1',
    '^boot/(.+)$': '<rootDir>/src/boot/$1',
    '^components/(.+)$': '<rootDir>/src/components/$1',
    '^src/utils/(.+)$': '<rootDir>/src/utils/$1',
  },
  globals: {
    'vue-jest': {
      compilerOptions: {
        isCustomElement: (tag) => tag.startsWith('q-'),
      },
    },
  },
};
