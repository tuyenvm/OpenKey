
import yaml
import os
import sys
from datetime import datetime
from pathlib import Path

# Config
BACKLOG_PATH = "records/active/backlog.yml"
TICKET_ROOT = "records/tickets/inbox"

def load_backlog():
    try:
        if not os.path.exists(BACKLOG_PATH):
            return {"active": []}
        with open(BACKLOG_PATH, 'r') as f:
            return yaml.safe_load(f) or {"active": []}
    except Exception as e:
        print(f"❌ Error loading backlog: {e}")
        return {"active": []}

def check_file_freshness(file_path, ref_time_str):
    if not os.path.exists(file_path): return False, "Missing"
    try:
        ref_time = datetime.fromisoformat(ref_time_str.replace('Z', '+00:00')).replace(tzinfo=None)
        file_mtime = datetime.fromtimestamp(os.path.getmtime(file_path))
        if file_mtime < ref_time:
            return False, "Stale (Older than Ticket)"
        return True, "Fresh"
    except:
        return True, "Unknown Time"

def audit_ticket(ticket):
    tid = ticket.get('id')
    title = ticket.get('title')
    print(f"\n🔎 Auditing Ticket: {tid} ('{title}')")
    
    score = 100
    violations = []

    # 1. WHO (Owner)
    owner = ticket.get('owner')
    if not owner or owner == "TBD":
        violations.append("❌ WHO: Owner is TBD or Missing.")
        score -= 10

    # 2. WHY (Mission/Alignment)
    # Checking metadata if available, otherwise superficial check
    if 'alignment' not in ticket and 'mission' not in ticket: 
         violations.append("⚠️  WHY: Missing Alignment/Mission.")
         score -= 5

    # 3. HOW (Plan Presence & Freshness)
    # Requires checking filesystem
    ticket_dir = os.path.join(TICKET_ROOT, tid)
    # Check global plan if local not found (MVP)
    plan_path = "implementation_plan.md" 
    started_at = ticket.get('started_at')
    
    if started_at:
        is_fresh, status = check_file_freshness(plan_path, started_at)
        if status == "Missing":
            violations.append("❌ HOW: Implementation Plan missing.")
            score -= 20
        elif status.startswith("Stale"):
             violations.append("⚠️  HOW: Plan is Stale (Older than Start Time).")
             score -= 10
    
    # 4. WHERE (Big Picture)
    # Hard to automate without parsing TICKET.md content, skipping for MVP script

    # 5. WHEN (Staleness)
    if started_at:
        try:
            start_date = datetime.fromisoformat(started_at.replace('Z', '+00:00')).replace(tzinfo=None)
            age = (datetime.now() - start_date).days
            if age > 7:
                violations.append(f"⚠️  WHEN: Ticket is {age} days old (Stale).")
                score -= 10
        except: pass

    # Report
    if violations:
        for v in violations:
            print(f"   {v}")
        print(f"   📉 Health Score: {score}/100")
    else:
        print("   ✅ All Checks Passed.")

def main():
    print("🦅 System Auditor: Universal Health Check")
    data = load_backlog()
    active_tickets = data.get('active', [])
    
    if not active_tickets:
        print("No active tickets to audit.")
        return

    print(f"Found {len(active_tickets)} active tickets.")
    for t in active_tickets:
        audit_ticket(t)

if __name__ == "__main__":
    main()
