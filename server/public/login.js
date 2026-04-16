// Login page logic — kept in a separate file so the Content Security Policy
// can allow 'self' scripts without needing 'unsafe-inline'.

const form        = document.getElementById('login-form');
const submitBtn   = document.getElementById('submit-btn');
const errorBanner = document.getElementById('error-banner');

function showError(message) {
  errorBanner.textContent = message;
  errorBanner.classList.add('visible');
}

function hideError() {
  errorBanner.classList.remove('visible');
}

form.addEventListener('submit', async (e) => {
  e.preventDefault();
  hideError();

  const username = document.getElementById('username').value.trim();
  const password = document.getElementById('password').value;

  submitBtn.disabled    = true;
  submitBtn.textContent = 'Signing in...';

  try {
    const res  = await fetch('/auth/login', {
      method:  'POST',
      headers: { 'Content-Type': 'application/json' },
      body:    JSON.stringify({ username, password }),
    });

    const data = await res.json();

    if (res.ok) {
      // Server set the httpOnly JWT cookie — redirect to the dashboard
      window.location.href = '/';
    } else {
      showError(data.error || 'Login failed. Please try again.');
    }
  } catch {
    showError('Could not reach the server. Check your connection.');
  } finally {
    submitBtn.disabled    = false;
    submitBtn.textContent = 'Sign In';
  }
});
