[Back](TechDoc_Architecture_Game_PurchaseTypes.md)

# Overview #

The Refill Purchase Type is used to give the purchase player health, armor and specific ammo counts back. Each Refill entry can list its own ratios, quantities, and ammo types.


# [Functionality](TechDoc_Architecture_Game_PurchaseTypes_Refill_Functionality.md) #


# XML Definition #
```
<Refill Health="1.0" Armor="0.0">
  <Ammo Name="Grenades" Amount="5" />
</Refill>
```

  * Health - Ratio [0,1] of percent of max health to give back to player, where 1.0 is full health.
  * Armor - Ratio [0,1] of percent of max armor to give back to player, where 0.5 is half armor.
  * Ammo - Ammo listing. The **Name** attribute is the type of ammo to reward, and the **Amount** attribute is how much of that ammo type to reward.


[Back](TechDoc_Architecture_Game_PurchaseTypes.md)