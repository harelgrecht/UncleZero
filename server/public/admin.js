// Admin page — Create User + list existing users.
// Redirects to /login if the session has expired.

const form      = document.getElementById('create-form');
const submitBtn = document.getElementById('submit-btn');
const banner    = document.getElementById('banner');

// ---------------------------------------------------------------------------
// Banner helpers
// ---------------------------------------------------------------------------

function showBanner(message, type) {
  banner.textContent = message;
  banner.className   = `banner visible ${type}`;
}

function hideBanner() {
  banner.className = 'banner';
}

// ---------------------------------------------------------------------------
// Load existing users
// ---------------------------------------------------------------------------

async function loadUsers() {
  const res = await fetch('/api/admin/users');

  if (res.status === 401) { window.location.href = '/login'; return; }

  const users = await res.json();
  const tbody  = document.getElementById('users-body');

  if (!users.length) {
    tbody.innerHTML = '<tr><td colspan="2" class="empty-users">No users yet.</td></tr>';
    return;
  }

  tbody.innerHTML = users.map(u => `
    <tr>
      <td>
        <span class="user-avatar">${u.username.charAt(0).toUpperCase()}</span>
        ${u.username}
      </td>
      <td class="created-at">${new Date(u.created_at).toLocaleDateString([], { year:'numeric', month:'short', day:'numeric' })}</td>
    </tr>
  `).join('');
}

// ---------------------------------------------------------------------------
// Create user form
// ---------------------------------------------------------------------------

form.addEventListener('submit', async (e) => {
  e.preventDefault();
  hideBanner();

  const username = document.getElementById('username').value.trim();
  const password = document.getElementById('password').value;
  const confirm  = document.getElementById('confirm').value;

  if (password !== confirm) {
    showBanner('Passwords do not match.', 'error');
    return;
  }

  submitBtn.disabled    = true;
  submitBtn.textContent = 'Creating...';

  try {
    const res  = await fetch('/api/admin/users', {
      method:  'POST',
      headers: { 'Content-Type': 'application/json' },
      body:    JSON.stringify({ username, password }),
    });

    const data = await res.json();

    if (res.status === 401) { window.location.href = '/login'; return; }

    if (res.ok) {
      showBanner(`User "${data.username}" created successfully.`, 'success');
      form.reset();
      loadUsers(); // refresh the list
    } else {
      showBanner(data.error || 'Failed to create user.', 'error');
    }
  } catch {
    showBanner('Could not reach the server. Check your connection.', 'error');
  } finally {
    submitBtn.disabled    = false;
    submitBtn.textContent = 'Create User';
  }
});

// ---------------------------------------------------------------------------
// Boot
// ---------------------------------------------------------------------------

loadUsers();
