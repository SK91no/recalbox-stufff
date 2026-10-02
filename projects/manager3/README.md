![](https://s3-eu-west-1.amazonaws.com/forums.recalbox.com/8d81e556-aefb-4729-ad2f-9d7386eff4cd.png)

<a href="https://vuejs.org/"><img src="https://img.shields.io/badge/vue-3.x-brightgreen.svg?style=flat-square" alt=""/></a>
<a href="https://quasar.dev/"><img src="https://img.shields.io/npm/v/quasar.svg?label=quasar" alt=""></a>
<a href="https://quasar.dev/"><img src="https://img.shields.io/npm/v/%40quasar/app.svg?label=@quasar/app" alt=""></a>
<a href="https://quasar.dev/"><img src="https://img.shields.io/npm/v/%40quasar/cli.svg?label=@quasar/cli" alt=""></a>
<a href="https://quasar.dev/"><img src="https://img.shields.io/npm/v/%40quasar/app-webpack?label=@quasar/app-webpack" alt=""></a>
<a href="https://quasar.dev/"><img src="https://img.shields.io/npm/v/%40quasar/extras.svg?label=@quasar/extras" alt=""></a>

[![Join the chat at https://discord.com/invite/NbQFbGM](https://img.shields.io/badge/chat-on%20discord-7289da.svg)](https://discord.com/invite/NbQFbGM)
<a href="https://forum.recalbox.com/" target="_blank"><img src="https://img.shields.io/badge/community-forum-brightgreen.svg"></a>

# Recalbox Manager v3

## Install Quasar CLI
```bash
yarn global add @quasar/cli
# or
npm i -g @quasar/cli
```


## Install the dependencies
```bash
yarn
# or
npm install

For build : set the env var 'API_IP' in your server config
```

## Create .env file on project root with your local API IP
```bash
API_IP=recalbox.local
```

## Start the app in development mode (hot-code reloading, error reporting, etc.)
```bash
quasar dev
```


## Lint the files
```bash
yarn lint
# or
npm run lint
```



## Build the app for production
```bash
quasar build
```

## Customize the configuration
See [Configuring quasar.config.mjs](https://v2.quasar.dev/quasar-cli-webpack/quasar-config-js).
