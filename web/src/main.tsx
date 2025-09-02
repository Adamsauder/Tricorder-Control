import React from 'react'
import ReactDOM from 'react-dom/client'
import App from './App.tsx'
import './index.css'

console.log('🚀 Main.tsx loading...')
console.log('📍 Root element exists:', !!document.getElementById('root'))

const root = document.getElementById('root')
if (root) {
  console.log('✅ Creating React root...')
  ReactDOM.createRoot(root).render(
    <React.StrictMode>
      <App />
    </React.StrictMode>,
  )
  console.log('✅ React root created and app rendered')
} else {
  console.error('❌ Root element not found!')
}
