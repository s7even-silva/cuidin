<script setup lang="ts">
</script>

<template>
  <header>
    <h1 class="titulo">Cuidín</h1>
    <nav>
      <RouterLink to="/conexion">Conexión</RouterLink>
      <RouterLink to="/umbrales">Umbrales</RouterLink>
      <RouterLink to="/sonidos">Sonidos</RouterLink>
      <RouterLink to="/estado">Estado</RouterLink>
    </nav>
  </header>
  <main>
    <RouterView />
  </main>
</template>

<style scoped>
header {
  display: flex;
  align-items: center;
  gap: 2rem;
  padding: 1rem 1.5rem;
  border-bottom: 1px solid #333;
}
.titulo {
  font-size: 1.2rem;
  margin: 0;
}
nav {
  display: flex;
  gap: 1rem;
}
nav a {
  color: inherit;
  text-decoration: none;
  opacity: 0.7;
}
nav a.router-link-active {
  opacity: 1;
  font-weight: 600;
}
main {
  max-width: 640px;
  margin: 0 auto;
  padding: 1.5rem;
}
</style>
