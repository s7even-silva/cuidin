import { defineStore } from 'pinia'
import { useBleConnection } from '../composables/useBleConnection'

export const useBleStore = defineStore('ble', () => {
  const conexion = useBleConnection()
  return conexion
})
