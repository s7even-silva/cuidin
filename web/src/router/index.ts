import { createRouter, createWebHashHistory } from 'vue-router'

const router = createRouter({
  history: createWebHashHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      redirect: '/conexion',
    },
    {
      path: '/conexion',
      name: 'conexion',
      component: () => import('../views/ConnectView.vue'),
    },
    {
      path: '/umbrales',
      name: 'umbrales',
      component: () => import('../views/ThresholdsView.vue'),
    },
    {
      path: '/sonidos',
      name: 'sonidos',
      component: () => import('../views/SoundsView.vue'),
    },
    {
      path: '/estado',
      name: 'estado',
      component: () => import('../views/StatusView.vue'),
    },
    {
      path: '/enfoque',
      name: 'enfoque',
      component: () => import('../views/FocusView.vue'),
    },
  ],
})

export default router
