import discord

ROSA = 0xDB00A6
BANNER = "https://i.imgur.com/DSxv3VU.png"

# ─── MAIN PANEL (ponto de entrada) ───

class MainPanel(discord.ui.View):
    def __init__(self):
        super().__init__(timeout=None)

    @discord.ui.button(label="Vendas", style=discord.ButtonStyle.success, custom_id="main_vendas", emoji="\U0001f4b0")
    async def vendas(self, i: discord.Interaction, b: discord.ui.Button):
        from auth_bot import criar_ticket
        view = SalesPlanSelect()
        await i.response.send_message("Selecione o plano desejado:", view=view, ephemeral=True)

    @discord.ui.button(label="Suporte", style=discord.ButtonStyle.secondary, custom_id="main_suporte", emoji="\U0001f4ac")
    async def suporte(self, i: discord.Interaction, b: discord.ui.Button):
        from auth_bot import criar_ticket
        await criar_ticket(i, "suporte")

# ─── SALES ───

class SalesPlanSelect(discord.ui.View):
    def __init__(self):
        super().__init__(timeout=None)

    @discord.ui.select(placeholder="Escolha seu plano...", options=[
        discord.SelectOption(label="Diario - R$15", description="1 dia de acesso", value="diario", emoji="\U0001f4a5"),
        discord.SelectOption(label="Semanal - R$50", description="7 dias de acesso", value="semanal", emoji="\U0001f550"),
        discord.SelectOption(label="Mensal - R$100", description="30 dias de acesso", value="mensal", emoji="\U0001f4c5"),
        discord.SelectOption(label="Lifetime - R$500", description="Acesso vitalicio", value="lifetime", emoji="\u26a1"),
    ])
    async def select_plan(self, i: discord.Interaction, s: discord.ui.Select):
        from auth_bot import criar_ticket
        await criar_ticket(i, "venda", s.values[0])

class SalesApproveView(discord.ui.View):
    def __init__(self):
        super().__init__(timeout=None)

    def _get_ticket(self, channel_id):
        from auth_bot import db_execone, using_pg
        sql = "SELECT * FROM tickets WHERE channel_id = %s AND status = 'open'" if using_pg else "SELECT * FROM tickets WHERE channel_id = ? AND status = 'open'"
        row = db_execone(sql, (str(channel_id),))
        if not row:
            return None
        return dict(row) if not isinstance(row, dict) else row

    @discord.ui.button(label="Fechar Ticket", style=discord.ButtonStyle.danger, custom_id="sales_close", emoji="\U0001f512")
    async def fechar(self, i: discord.Interaction, b: discord.ui.Button):
        from auth_bot import is_admin
        if not is_admin(i):
            return await i.response.send_message("Sem permissao.", ephemeral=True)
        view = SuporteConfirmClose()
        await i.response.send_message("Tem certeza que deseja fechar este ticket?", view=view, ephemeral=True)

    @discord.ui.button(label="Fechar Ticket (Forçar)", style=discord.ButtonStyle.danger, custom_id="sales_force_close", emoji="\u26a0\ufe0f", row=1)
    async def force_close(self, i: discord.Interaction, b: discord.ui.Button):
        from auth_bot import is_admin, db_exec, db_commit, db_execone, using_pg
        if not is_admin(i):
            return await i.response.send_message("Sem permissao.", ephemeral=True)
        await i.response.defer(ephemeral=True)
        channel = i.channel
        sql = "SELECT id FROM tickets WHERE channel_id = %s" if using_pg else "SELECT id FROM tickets WHERE channel_id = ?"
        row = db_execone(sql, (str(channel.id),))
        tid = row["id"] if row and isinstance(row, dict) else (row[0] if row else None)
        if tid:
            usql = "UPDATE tickets SET status = 'closed' WHERE id = %s" if using_pg else "UPDATE tickets SET status = 'closed' WHERE id = ?"
            db_exec(usql, (tid,))
            if not using_pg:
                db_commit()
        await i.followup.send("Fechando canal forcadamente...")
        try:
            await channel.delete()
        except Exception as e:
            await i.followup.send(f"Erro ao deletar: {e}")

    @discord.ui.button(label="Aprovar Pagamento", style=discord.ButtonStyle.success, custom_id="approve_payment", emoji="\u2705")
    async def approve(self, i: discord.Interaction, b: discord.ui.Button):
        from auth_bot import is_admin, db_exec, db_commit, using_pg, now, gen_key, CONFIG, bot
        if not is_admin(i):
            return await i.response.send_message("Sem permissao.", ephemeral=True)
        await i.response.defer(ephemeral=True)

        ticket = self._get_ticket(i.channel.id)
        if not ticket:
            return await i.followup.send("Nenhum ticket aberto neste canal.", ephemeral=True)

        target_id = int(ticket.get("user_id", 0))
        plan_dias = {"diario": 1, "semanal": 7, "mensal": 30, "lifetime": 36500}
        dias = plan_dias.get(ticket.get("plan", ""), 30)
        k = gen_key()
        if using_pg:
            db_exec("INSERT INTO keys (key, duration_days, used_by, used_at, created_at) VALUES (%s, %s, %s, %s, %s)",
                    (k, dias, i.user.name, now(), now()))
            db_exec("UPDATE tickets SET status = 'approved', payment_status = 'paid' WHERE id = %s", (ticket["id"],))
        else:
            db_exec("INSERT INTO keys (key, duration_days, used_by, used_at, created_at) VALUES (?, ?, ?, ?, ?)",
                    (k, dias, i.user.name, now(), now()))
            db_exec("UPDATE tickets SET status = 'approved', payment_status = 'paid' WHERE id = ?", (ticket["id"],))
            db_commit()

        member = i.guild.get_member(target_id)
        if member:
            try:
                embed_dm = discord.Embed(
                    title="\u2705 Pagamento Aprovado!",
                    color=discord.Color.green(),
                    description=(
                        f"**Sua key:** `{k}`\n"
                        f"**Duração:** {dias} dias\n\n"
                        f"Use o loader para acessar o Satella.\n"
                        f"Obrigado pela preferencia! \u2764"
                    )
                )
                embed_dm.set_image(url=BANNER)
                embed_dm.set_footer(text="Satella Private")
                await member.send(embed=embed_dm)
            except:
                pass

        embed = discord.Embed(
            title="\u2705 Pagamento Aprovado",
            color=discord.Color.green(),
            description=(
                f"**Admin:** {i.user.mention}\n"
                f"**Cliente:** <@{target_id}>\n"
                f"**Key:** `{k}`\n"
                f"**Duração:** {dias} dias\n\n"
                f"\U0001f4e5 Key enviada no privado do cliente."
            )
        )
        await i.channel.send(embed=embed)
        await i.followup.send(f"Pagamento aprovado! Key enviada.", ephemeral=True)
        await discord.utils.sleep(3)
        try:
            await i.channel.delete()
        except:
            await i.channel.send("Nao foi possivel deletar o canal (permissoes). Feche manualmente.")

    @discord.ui.button(label="Negar Pagamento", style=discord.ButtonStyle.danger, custom_id="deny_payment", emoji="\u274c")
    async def deny(self, i: discord.Interaction, b: discord.ui.Button):
        from auth_bot import is_admin, db_exec, db_commit, using_pg
        if not is_admin(i):
            return await i.response.send_message("Sem permissao.", ephemeral=True)
        await i.response.defer(ephemeral=True)

        ticket = self._get_ticket(i.channel.id)
        if not ticket:
            return await i.followup.send("Nenhum ticket aberto neste canal.", ephemeral=True)

        target_id = int(ticket.get("user_id", 0))
        if using_pg:
            db_exec("UPDATE tickets SET status = 'denied' WHERE id = %s", (ticket["id"],))
        else:
            db_exec("UPDATE tickets SET status = 'denied' WHERE id = ?", (ticket["id"],))
            db_commit()

        member = i.guild.get_member(target_id)
        if member:
            try:
                embed_dm = discord.Embed(
                    title="\u274c Pagamento Recusado",
                    color=discord.Color.red(),
                    description=(
                        "Seu pagamento foi recusado.\n"
                        "Entre em contato com o suporte para mais informacoes."
                    )
                )
                embed_dm.set_footer(text="Satella Private")
                await member.send(embed=embed_dm)
            except:
                pass

        embed = discord.Embed(
            title="\u274c Pagamento Recusado",
            color=discord.Color.red(),
            description=f"**Admin:** {i.user.mention}\n**Cliente:** <@{target_id}>\n\nPagamento negado."
        )
        await i.channel.send(embed=embed)
        await i.followup.send("Pagamento recusado.", ephemeral=True)
        await discord.utils.sleep(3)
        try:
            await i.channel.delete()
        except:
            await i.channel.send("Nao foi possivel deletar o canal (permissoes). Feche manualmente.")

# ─── SUPORTE ───

class SuporteClose(discord.ui.View):
    def __init__(self):
        super().__init__(timeout=None)

    @discord.ui.button(label="Fechar Ticket", style=discord.ButtonStyle.danger, custom_id="suporte_close", emoji="\U0001f512")
    async def close(self, i: discord.Interaction, b: discord.ui.Button):
        from auth_bot import is_admin
        if not is_admin(i):
            return await i.response.send_message("Apenas administradores podem fechar tickets.", ephemeral=True)
        view = SuporteConfirmClose()
        await i.response.send_message("Tem certeza que deseja fechar este ticket?", view=view, ephemeral=True)

class SuporteConfirmClose(discord.ui.View):
    def __init__(self):
        super().__init__(timeout=None)

    @discord.ui.button(label="Sim, fechar", style=discord.ButtonStyle.danger, custom_id="suporte_confirm_yes", emoji="\u2705")
    async def confirm(self, i: discord.Interaction, b: discord.ui.Button):
        from auth_bot import db_exec, db_commit, db_execone, using_pg
        channel = i.channel
        sql = "SELECT id FROM tickets WHERE channel_id = %s AND status = 'open'" if using_pg else "SELECT id FROM tickets WHERE channel_id = ? AND status = 'open'"
        row = db_execone(sql, (str(channel.id),))
        tid = row["id"] if row and isinstance(row, dict) else (row[0] if row else None)
        if tid:
            usql = "UPDATE tickets SET status = 'closed' WHERE id = %s" if using_pg else "UPDATE tickets SET status = 'closed' WHERE id = ?"
            db_exec(usql, (tid,))
            if not using_pg:
                db_commit()
        await i.response.send_message("Fechando ticket em 5 segundos...")
        await discord.utils.sleep(5)
        try:
            await channel.delete()
        except Exception as e:
            try:
                await channel.send(f"Erro ao deletar: {e}. Feche manualmente.")
            except:
                pass

    @discord.ui.button(label="Cancelar", style=discord.ButtonStyle.secondary, custom_id="suporte_confirm_no", emoji="\u274c")
    async def cancel(self, i: discord.Interaction, b: discord.ui.Button):
        await i.response.send_message("Fechamento cancelado.", ephemeral=True)
