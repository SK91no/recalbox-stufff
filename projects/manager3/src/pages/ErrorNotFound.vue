<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div class="notFound fullscreen ">
    <div class="background-container text-white text-center flex flex-center">
      <div>
        <div style="font-size: 30vh">
          404
        </div>

        <div class="text-h2" style="opacity:.4">
          Oops. Nothing here...
        </div>

        <q-btn
          class="q-mt-xl"
          color="white"
          text-color="primary"
          unelevated
          to="/"
          label="Go Home"
          no-caps
        />
      </div>
    </div>
  </div>
</template>

<script lang="ts" setup>

</script>

<style lang="sass">
.notFound
  background-image: linear-gradient(to right, $primary, $accent)

  @keyframes animatedBackground
    from
      background-position: 0 0
    to
      background-position: 100% 0

  .background-container
    height: 100%
    background-image: url(../assets/controller-background.png)
    position: relative
    animation: animatedBackground 200s linear infinite alternate
    text-shadow: 0 0 16px #00000061
</style>
